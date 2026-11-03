$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot '../tools/pal4_movement_plan.ps1')
$plan=@(ConvertFrom-Pal4MovementPlan '[{"key":"D","samples":75},{"key":"W","samples":515},{"key":"W+A","samples":1}]')
if ($plan.Count -ne 3 -or $plan[1].Key -ne 'W' -or $plan[1].Samples -ne 515 -or $plan[2].Key -ne 'W+A') {
    throw 'Plan changed action order, keys or sample counts.'
}
foreach($invalid in @('[]','{}','[null]','[{"key":"ESC","samples":1}]',
    '[{"key":"W","samples":0}]','[{"key":"W","samples":601}]',
    '[{"key":"W","samples":1.5}]','[{"key":"W","samples":"1"}]',
    '[{"key":"W+W","samples":1}]','[{"key":"w","samples":1}]',
    '[{"key":"W","samples":1,"extra":1}]',('['+((1..65|ForEach-Object {'{"key":"W","samples":1}'}) -join ',')+']'))) {
    $rejected=$false
    try { $null=@(ConvertFrom-Pal4MovementPlan $invalid) } catch { $rejected=$true }
    if(-not $rejected){throw "Invalid plan accepted: $invalid"}
}
Write-Output 'PASS: ordered variable sample counts, chords and invalid/bounded plan rejection'
$scene='{"world_state_active":true,"control_enabled":1,"combat":{"players":[],"monsters":[]}}' | ConvertFrom-Json
Assert-Pal4SceneMovementState $scene
foreach($invalidState in @(
    '{"world_state_active":false,"control_enabled":1,"combat":{"actors":[]}}',
    '{"world_state_active":true,"control_enabled":0,"combat":{"actors":[]}}',
    '{"world_state_active":true,"control_enabled":1,"combat":{"players":[{"id":1001}],"monsters":[]}}',
    '{"world_state_active":true,"control_enabled":1,"combat":{"actors":[{"id":1001}]}}',
    '{"control_enabled":1,"combat":{"actors":[]}}')) {
    $rejected=$false
    try{Assert-Pal4SceneMovementState ($invalidState | ConvertFrom-Json)}catch{$rejected=$true}
    if(-not $rejected){throw 'System/locked/Combat/old snapshot was accepted as free Scene movement.'}
}
Write-Output 'PASS: active Scene identity, lock, original/native roster and missing-field guards'

$mixed=@(ConvertFrom-Pal4MovementPlan '[{"key":"A","samples":3},{"key":"S","hold_ms":850},{"key":"W+D","hold_ms":50}]')
if($mixed.Count -ne 3 -or $mixed[0].Samples -ne 3 -or $mixed[0].HoldMs -ne 0 -or
   $mixed[1].Samples -ne 0 -or $mixed[1].HoldMs -ne 850 -or $mixed[2].Key -ne 'W+D') {
    throw 'Mixed plan lost sampled turn/timed movement mode, order or duration.'
}
foreach($invalid in @('[{"key":"W","samples":3,"hold_ms":100}]','[{"key":"W","hold_ms":49}]',
    '[{"key":"W","hold_ms":10001}]','[{"key":"W","hold_ms":50.5}]','[{"key":"W","hold_ms":"500"}]',
    '[{"key":"W","duration":500}]')) {
    $rejected=$false
    try{$null=@(ConvertFrom-Pal4MovementPlan $invalid)}catch{$rejected=$true}
    if(-not $rejected){throw "Invalid timed plan accepted: $invalid"}
}
Write-Output 'PASS: mixed sampled turns/timed holds preserve modes and bounds; ambiguous duration rejected'
