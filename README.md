
# Public Repository
This is the public repository for plugins / modules built exclusively for War of Being. These are typically boilerplate and/or very early versions of the systems we build and are made public for educational purposes. Often, these systems are iterated on and things get changed up internally, so please do not submit issues or help requests as I do not track or offer support for these versions.


# WobGasSystem

Early template for War of Being's gameplay system that includes support for Epic Games' Gameplay Ability System Plugin.  

## Modules
### WobGasSystem
The core module of this plugin integrates support for Epic's GAS plugin, with basic attributes including Character health, experience and levels, Death detection and appropriate delegates. Useful as a starting point for almost any GAS character.

### WobGasVRExpansion
A character class created in WobGasSystem that integrates support for Joshua (MordenTral) Statzer's VRExpansion Plugin. Includes transcoded logic for the gripping system from MordenTral's blueprint example into native C++, including grips, sockets and climbing logic.  Some customisations have been made to more closely fit the projects purposes and iron out some smooth movement issues.

## Dependencies
The VR module has dependency on the VRExpansion plugin by Joshua (MordenTral) Statzer and should be disabled if not installed/required to successfully build the source code. 

### Thanks for peeking!
