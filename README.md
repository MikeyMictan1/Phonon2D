<h1 align="center">Phonon2D : 2D Acoustic Ray-Tracing as a Game Mechanic for the Godot Engine </h1>

<div align="center">
<img width="400" height="400" alt="Phonon2D Logo" src="https://github.com/user-attachments/assets/cf8a1bdf-fe73-445d-ac4d-acd685901715" />
</div>

## Abstract
Audio in video games is largely implemented through a distance-based falloff model where volume decreases as a simple function of distance from the source, disconnected from the geometry and materials of the environment. Despite sound being a fundamental human sense, it is rarely treated as a core game mechanic, instead often serving as an accompaniment.

While audio middleware such as Wwise and FMOD offer spatial audio features, these tools are primarily designed for 3D. They also focus on atmospheric realism and do not expose their sound logic in a way that allows developers to use it for gameplay. In 2D engines support is even sparser, and building such a system needs significant ground-up development.

This project presents Phonon2D, a C++ GDExtension library for the Godot engine that brings physically-modelled sound propagation to 2D games. Rather than relying on conventional distance-based audio falloff, Phonon2D simulates sound as discrete energy packets (phonons) that are cast outward from a source, travel through the environment, and interact with surfaces based on their material properties. Phonons reflect off hard surfaces, get absorbed by soft ones, scatter through uneven materials, and lose energy over distance, resulting in audio that is shaped by the geometry and materials of the scene itself.

The library ray-traces phonon packets from emitter nodes, applying energy decay from distance travelled and impact with configurable phonon materials. A singleton listener converts arriving phonons into audible output, while dedicated sensor nodes evaluate incoming phonons against gameplay conditions and can trigger game events in response.

Phonon2D provides developers with an open-source toolset for integrating complex audio into 2D games without needing to build custom systems, enabling a category of sound-driven game mechanics that can be built within Godot.



## Tech Stack
**Languages** : C++, GDScript <br>
**Engine** : Godot <br>
**Software Type**: GDExtension - A library of Phonon2D nodes <br>
**Dimension** : 2D (for now) <br>

## Library Structure
`Phonon Packet`: Object that acts as the sound bouncing around.

`PhononEmitter2D`: Inherits from AudioStreamPlayer2D, is what ‘makes’ the phonons at a point, whenever the sound is triggered. Can customise volume, direction, etc.

`PhononListener2D`: Only one in the scene, is how the audio is processed + how loud it will actually sound.

`PhononSensor2D`: Detects phonons, and emits signals if certain conditions are met e.g. frequency, intensity, time-window (what allows the game mechanic side of things).

`PhononMaterial`: Material resource that can be applied to 2D sprites and tilemaps, which defines how that material interacts with sound e.g. foam being very absorbing.

`Absorption Type`: PhononMaterial property, deciding how absorbing the material is based on material options + parameters.


## Architecture (Outdated)
<img width="3055" height="2160" alt="image" src="https://github.com/user-attachments/assets/f54f17ad-3cd4-4292-b3bb-5823fd3fdc81" />
