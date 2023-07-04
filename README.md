# AMD Microphone
AMD Digital Microphone kext for modern AMD laptops with AMD's Audio Co-Processor (ACP). 

<p align="center">
  <img src="./screenshot.png" width="70%">
</p>

## What does this kext do?
In modern AMD laptops, microphones are no longer connected to High Definition Audio bus.
They are now controlled by a dedicated Audio Co-Processor, hence obviously no driver in macOS.
This kext brings support for them. Refer #FAQ for more information.

## What's working?
* Microphone, of course!

## What's not working?
* Volume control
* Mute

## Prerequisites 
### Disable SIP
This kext relies on `com.apple.iokit.IOAudioFamily`, which somehow is loaded very late in the boot process, so I cannot use this with OpenCore.
At the moment the only way to install this kext is to place it under `/Library/Extensions/`, which requires SIP to be disabled.
I may find a solution for this in the future.

### Check if your laptop is equipped with an Audio Co-Processor.
T.B.D

## Installation
1. Disable SIP.
2. Download the kext from Github Actions.
3. Copy it to `/Library/Extensions/`.
4. When notified, allow it in `Security & Privacy settings` and reboot.

## FAQ
#### I heard that AMD Microphone can work with VoodooHDA. How is this kext different?

Modern AMD laptops no longer use High Definition Audio (HDA). It has a dedicated Audio Co-Processor for controlling microphones.

#### Can I use this kext along with AppleALC?

Yes. This kext is a separate driver for microphones. You still need AppleALC for audio output.
