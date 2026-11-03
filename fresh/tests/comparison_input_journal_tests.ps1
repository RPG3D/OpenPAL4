param([string]$OutputRoot = 'H:/PAL4/objs')
$ErrorActionPreference = 'Stop'

# Import only the journal function, without launching or sending input to games.
$errors = $null
$source = Join-Path $PSScriptRoot '../tools/compare_pal4_side_by_side.ps1'
$tree = [Management.Automation.Language.Parser]::ParseFile(
    $source, [ref]$null, [ref]$errors)
if ($errors.Count) { throw $errors }
$definition = $tree.Find({ param($node)
    $node -is [Management.Automation.Language.FunctionDefinitionAst] -and
        $node.Name -eq 'Save-PairInputLog'
}, $true)
if (-not $definition) { throw 'Comparison journal function is missing.' }
. ([ScriptBlock]::Create($definition.Extent.Text))

$directory = Join-Path $OutputRoot ('input-journal-tests-' + [Guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($directory) | Out-Null
$inputLog = Join-Path $directory 'input.json'
$inputEvents = [Collections.Generic.List[object]]::new()
Save-PairInputLog
if ((Get-Content $inputLog -Raw) -notmatch '^\[\s*\]$') {
    throw 'Empty journal must be a JSON array.'
}
$inputEvents.Add([pscustomobject]@{ Message = 34058; LParam = 40 })
Save-PairInputLog
$inputEvents.Add([pscustomobject]@{ Message = 34058; LParam = 0 })
$lock = [IO.File]::Open($inputLog, [IO.FileMode]::Open,
    [IO.FileAccess]::Read, [IO.FileShare]::None)
$failed = $false
try { Save-PairInputLog } catch {
    $failed = $_.Exception.Message -match 'Do not replay input'
} finally { $lock.Dispose() }
if (-not $failed) { throw 'Locked publication must report preserved successful sends.' }
$previous = Get-Content $inputLog -Raw | ConvertFrom-Json
if ($previous.Count -ne 1 -or $previous[0].LParam -ne 40) {
    throw 'Failed publication damaged the last good journal.'
}
$retained = @(Get-ChildItem $directory -Filter '*.tmp')
if ($retained.Count -ne 1) { throw 'Failed publication must preserve its complete temporary.' }
$pending = Get-Content $retained[0].FullName -Raw | ConvertFrom-Json
if ($pending.Count -ne 2 -or $pending[1].LParam -ne 0) {
    throw 'Failed publication lost successful-send records.'
}
Save-PairInputLog
$published = Get-Content $inputLog -Raw | ConvertFrom-Json
if ($published.Count -ne 2 -or $published[1].LParam -ne 0 -or $inputEvents.Count -ne 2) {
    throw 'Publication after unlocking changed or lost input records.'
}
Write-Output "PASS: journal array shape, locked-file preservation and later publication; artifacts=$directory"

# Exercise the actual driver's boundary gate without a running game or input.
$waitDefinition = $tree.Find({ param($node)
    $node -is [Management.Automation.Language.FunctionDefinitionAst] -and
        $node.Name -eq 'Wait-PairKeyboardBudget'
}, $true)
if (-not $waitDefinition) { throw 'Keyboard budget wait function is missing.' }
. ([ScriptBlock]::Create($waitDefinition.Extent.Text))
function Start-Sleep { param([int]$Milliseconds) }
function Save-RebuiltMovementState { param($Name,[switch]$BudgetOnly) }
function Read-PairKeyboardBudget {
    param($Name)
    $script:budgetBoundaryReasons = $script:testBoundaryReasons
    return @(0,0)
}
$FastMovementObservation = $false
foreach ($reasons in @(@(2,0),@(0,1),@(2,2))) {
    $script:testBoundaryReasons = $reasons
    $caughtBoundary = $false
    try { Wait-PairKeyboardBudget 1 30 } catch {
        $caughtBoundary = $_.Exception.Message -match 'no later planned input sent'
    }
    if (-not $caughtBoundary) { throw 'Either-side boundary must stop even when both budgets have drained.' }
}
$script:testBoundaryReasons = @(0,0)
Wait-PairKeyboardBudget 1 30
Write-Output 'PASS: either-side boundary stops later plan steps; ordinary completion succeeds.'
