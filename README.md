# AMD Microphone
AMD Digital Microphone kext for modern AMD laptops with AMD's Audio Co-Processor (ACP).

## NOTE: For non-ACP microphones, there is an experimental fix [here](https://github.com/qhuyduong/AppleALC)

<p align="center">
  <img src="https://github.com/qhuyduong/AMDMicrophone/assets/16358771/4e2b9655-e53a-4433-a1f4-9b926bcec14b" width="70%">
</p>

## What does this kext do?
In some modern AMD laptops, microphones are no longer connected to High Definition Audio bus.
They are now controlled by a dedicated Audio Co-Processor, hence obviously no driver in macOS.
This kext brings support for them. Refer [FAQ](#faq) for more information.

## What's working?
* Microphone for Renoir-base laptops with digital microphone (aka ACP-based mic), see [this](#check-if-your-laptop-is-equipped-with-an-acp-based-microphone) to identify if your laptop is supported.

## What's not working?
* Other CPU variants than Renoir, such as Raven/Raven2/Fire Flight/Van Gogh/Yellow Carp. Please open issues if you want support for these devices.

## Prerequisites 
### Check if your laptop is equipped with an ACP-based microphone
#### Windows
Make sure you have a working Microphone Array device.

<p align="center">
  <img src="https://github.com/qhuyduong/AMDMicrophone/assets/16358771/bc4ccf6b-35e6-49ac-b6ac-18c794600d3f" width="40%">
</p>

#### Linux
Make sure default source device is ACP

```console
$ pactl get-default-source | grep acp
alsa_input.pci-0000_03_00.6.HiFi__hw_acp__source
```

### Update csr-active-config
This kext relies on `com.apple.iokit.IOAudioFamily`, which somehow is loaded very late in the boot process, so we cannot add this kext directly to OpenCore.
At the moment the only way to install this kext is to place it under `/Library/Extensions/`, which requires kexts to be signed. The workaround is to set `csr-active-config` in OpenCore NVRAM settings to `01000000` (aka `CSR_ALLOW_UNTRUSTED_KEXTS`).


## Installation (follow in the right order)
1. Set `csr-active-config` to `01000000`.
2. Download the kext from Github Actions.
3. Copy it to `/Library/Extensions/`.

```console
sudo cp -r AMDMicrophone.kext /Library/Extensions/
```

4. When notified, allow it in `Security & Privacy` settings and reboot.

<p align="center">
  <img src="https://github.com/qhuyduong/AMDMicrophone/assets/16358771/58f053d7-7471-4cda-b9c1-fadd2de328aa" width="50%">
</p>

## FAQ
#### I heard that AMD Microphone can work with VoodooHDA. How is this kext different?
Some modern AMD laptops no longer use High Definition Audio (HDA). It has a dedicated Audio Co-Processor for controlling microphones.

#### Can I use this kext along with AppleALC?
Yes. This kext is a separate driver for microphones. You still need AppleALC for audio output.

#### After installing the kext, Microphone appears in the settings but no sound
Refer [this](#check-if-your-laptop-is-equipped-with-an-acp-based-microphone) to make sure your device is supported.
