![Coral Engine](https://github.com/user-attachments/assets/aade2fbe-e69b-42ac-8b51-98d9ab0cd3c1)

[*Watch the trailer*](https://youtu.be/Z4UFHaJ_ulQ?si=YbU7SAVAXNSH-myE)

## What is Coral Engine?

Coral Engine is a game engine developed by students of [Breda University of Applied Sciences](https://www.buas.nl/en/programmes/creative-media-and-game-technologies). It was designed to be accesible to designers, so that entire games can be created without having to modify a line of C++.

The 'main' branch of this repository represents the collective effort of various students, and included the code for Lichgate. To ensure each students work is preserved at this URL, **active development of Coral is done on [the main-lite branch](https://github.com/GuusKemperman/CoralEngine/tree/main-lite)**.

### Key Features

#### Graphics
- **Graphics Pipeline**: Built using DX12 for PC.
- **Shadows and Lighting**: Supports tens of thousands of point lights with minimal performance impact.
- **Post Processing**: Includes effects like fog, LUTs, outlines, and more.
- **Animated Skinned Meshes**: Supports complex animations for detailed character models.

#### ECS (Entity Component System)
- **Efficient Systems**: Utilizes [EnTT](https://github.com/skypjack/entt) for creating systems that iterate over entity views for optimal performance.
- **Event Handling**: Includes a variety of events like "OnBeginPlay", "OnTick", and "OnCollisionEntry" for flexible gameplay logic.

#### Level Editor
- **Built-In Level Editor**: Allows inspection and modification of all components and entities within a world.
- **Prefab System**: Uses a system similar to [Unity's prefabs](https://docs.unity3d.com/Manual/Prefabs.html).
- **Gizmos and Shortcuts**: Provides tools for quickly transforming objects and undoing changes.

#### Visual Scripting
- **Visual Script Editor**: Inspired by Unreal Engine's Blueprints, enabling the creation and modification of scripts at runtime.
- **Seamless Integration**: Built from scratch to integrate smoothly with the engine and EnTT, allowing scripts to benefit from ECS's data-oriented speed.

#### Additional Features
- **Animation Blending**: For smooth transitions between animations.
- **Audio**: Built-in support for game audio.

## Projects Created with Coral Engine
- [Lichgate](https://buas.itch.io/lichgate)
- [Ant Simulation](https://github.com/GuusKemperman/Ants)

## Getting Started

1. **Set Startup Project**: Right-click on `ExampleGame` and select "Set as startup project."
2. **Configure Build**: By default, Visual Studio selects Debug mode. For the full editor, choose `EditorDebug` or `EditorRelease`.

## Starting a New Project

1. **Duplicate ExampleGame Folder**: Copy the `Games/ExampleGame` folder.
2. **Clean Up**: Delete the contents of `Games/ExampleGame/Assets` and all files in `Games/ExampleGame/Include` and `Games/ExampleGame/Source` except `Main.cpp`.
3. **Rename Project**: Rename the folder and project to your desired game name.
4. **Include in Solution**: Add the new project to `Coral.sln`.
