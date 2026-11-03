# Execute the launcher's actual environment scopes with Start-Process mocked.
# No game, launcher executable, UI or build is started.
$ErrorActionPreference = 'Stop'
$launcherPath = Join-Path $PSScriptRoot '../tools/compare_pal4_side_by_side.ps1'
$parseTokens = $null; $parseErrors = $null
$ast = [System.Management.Automation.Language.Parser]::ParseFile(
    (Resolve-Path $launcherPath), [ref]$parseTokens, [ref]$parseErrors)
if ($parseErrors.Count) { throw 'Comparison launcher syntax failed' }
$scopes = @($ast.FindAll({param($node)
    $node -is [System.Management.Automation.Language.TryStatementAst] -and
    ($node.Body.Extent.Text -match '\$(launcher|rebuiltProcess) = Start-Process')
}, $true))
if ($scopes.Count -ne 2) { throw 'Expected both original/native launch environment scopes' }
$validation = ($ast.EndBlock.Statements | Select-Object -First 6 | ForEach-Object { $_.Extent.Text }) -join "`n"
$ActionKeys = @(); $MovementKeys = @(); $DialogueSteps = 0
$MovementChord = ''; $MovementPlanFile = ''; $ClientClickX = -1
foreach ($invalid in @('valid', 'no-script', 'no-clock', 'continue', 'no-save', 'gameplay-input', 'startup-no-policy')) {
    $FrameGateStartupRequest = 123
    $FrameGateCheckpoint = $true; $FrameGate = $false
    $ScriptedKeyboard = $true; $ComparisonFrameStepMs = 17
    $ContinueExisting = $false; $LoadSaveSlot = 1; $ActionKeys = @()
    switch ($invalid) {
        'startup-no-policy' { $FrameGateCheckpoint = $false }
        'no-script' { $ScriptedKeyboard = $false }
        'no-clock' { $ComparisonFrameStepMs = 0 }
        'continue' { $ContinueExisting = $true }
        'no-save' { $LoadSaveSlot = -1 }
        'gameplay-input' { $ActionKeys = @('SPACE') }
    }
    $rejected = $false
    try { & ([scriptblock]::Create($validation + "`nif (-not `$FrameGate) { throw 'checkpoint must imply gate' }")) }
    catch { $rejected = $true }
    if ($rejected -ne ($invalid -ne 'valid')) { throw "Checkpoint launcher validation failed: $invalid" }
}
$ActionKeys = @()
$priorStartupValue = $env:PAL4_COMPARE_FRAME_GATE_STARTUP_REQUEST
$priorValue = $env:PAL4_COMPARE_FRAME_GATE_CHECKPOINT
try {
    function Start-Process {
        $script:observedCheckpoint = $env:PAL4_COMPARE_FRAME_GATE_CHECKPOINT
        $script:observedStartup = $env:PAL4_COMPARE_FRAME_GATE_STARTUP_REQUEST
        $script:launchCalls++
        if ($script:simulateFailure) { throw 'mock process creation failure' }
        return [pscustomobject]@{ Id = 123 }
    }
    $GameRoot = $PSScriptRoot
    $FrameGate = $true; $ScriptedKeyboard = $true; $ComparisonFrameStepMs = 17
    foreach ($scope in $scopes) {
        foreach ($enabled in @($true, $false)) {
            foreach ($inherited in @($null, 'inherited-marker')) {
                foreach ($failure in @($false, $true)) {
                    $FrameGateStartupRequest = if ($enabled) { 456 } else { 0 }
                    $FrameGateCheckpoint = $enabled
                    $priorComparisonFrameGateStartupRequest = $inherited
                    $env:PAL4_COMPARE_FRAME_GATE_STARTUP_REQUEST = $inherited
                    $priorComparisonFrameGateCheckpoint = $inherited
                    $env:PAL4_COMPARE_FRAME_GATE_CHECKPOINT = $inherited
                    $script:observedCheckpoint = 'not-called'; $script:launchCalls = 0
                    $script:simulateFailure = $failure
                    $threw = $false
                    try { & ([scriptblock]::Create($scope.Extent.Text)) }
                    catch {
                        if ($_.Exception.Message -ne 'mock process creation failure') { throw }
                        $threw = $true
                    }
                    $expectedStartup = if ($enabled) { "456" } else { $null }
                    if ($script:observedStartup -cne $expectedStartup -or $env:PAL4_COMPARE_FRAME_GATE_STARTUP_REQUEST -cne $inherited) { throw "Startup request opt-in/restoration failed" }
                    $expected = if ($enabled) { '1' } else { $null }
                    if ($script:launchCalls -ne 1 -or $script:observedCheckpoint -cne $expected) {
                        throw 'Checkpoint policy did not match explicit switch at process creation'
                    }
                    if ($env:PAL4_COMPARE_FRAME_GATE_CHECKPOINT -cne $inherited -or $threw -ne $failure) {
                        throw 'Checkpoint environment restoration failed on success or exception'
                    }
                }
            }
        }
    }
    'Original/native launcher checkpoint environment: explicit opt-in, inherited cleanup and exception restoration pass'
} finally {
    $env:PAL4_COMPARE_FRAME_GATE_STARTUP_REQUEST = $priorStartupValue
    $env:PAL4_COMPARE_FRAME_GATE_CHECKPOINT = $priorValue
}
