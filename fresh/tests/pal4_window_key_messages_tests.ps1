$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '../tools/pal4_window_key_messages.ps1')
foreach ($arrow in @(@(37,0x4b),@(38,0x48),@(39,0x4d),@(40,0x50))) {
    $plain = New-Pal4WindowKeyMessage $arrow[0] $arrow[1]
    $extended = New-Pal4WindowKeyMessage $arrow[0] ($arrow[1] -bor 0xe000)
    if (($plain.Down -band 0x01000000) -eq 0 -or $plain.Down -ne $extended.Down -or
        ($plain.Up -band 0xc0000000) -ne 0xc0000000 -or
        ($plain.Up -band 0x01ff0001) -ne $plain.Down) {
        throw 'Navigation arrows must retain the same extended scan/transition bits with or without an E0 map prefix.'
    }
}
foreach ($ordinary in @(@(87,0x11),@(13,0x1c),@(27,1))) {
    $message = New-Pal4WindowKeyMessage $ordinary[0] $ordinary[1]
    if (($message.Down -band 0x01000000) -ne 0 -or
        (($message.Down -shr 16) -band 0xff) -ne $ordinary[1]) {
        throw 'Ordinary WASD/Enter/Esc must retain their nonextended scan code.'
    }
}
if (((New-Pal4WindowKeyMessage 13 0xe01c).Down -band 0x01000000) -eq 0) {
    throw 'An explicit E0 map prefix must also be retained for a non-arrow key.'
}
Write-Output 'PASS: arrow layout fallback, E0 preservation, ordinary keys and key-up transition bits'
