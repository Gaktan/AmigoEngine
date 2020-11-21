# AmigoEngine

# Compiling

1. Run FirstSetup.bat to first get and compile dependencies.
2. Run GenerateProjects to generate VS projects and solutions.
3. You are ready to compile

# TODO List

- Material system
	- Think about it, how to we translate obj materials to shaders and textures automatically?
	- Manually create files to link material name to shader?
	- Export plugin for blender?
	- Link material and textures too

	Obj material-> only keep the name
	Material library-> match with the obj material name
	Material fetched from library-> reference to a shader object, defines constants, defines textures
	Shader object->Holds shader code, holds shader bindings
	Shader bindings-> Allows to bind resources to a shader (constant buffers, textures, ...)

- Keyboard controls

- Implement imGUI
	- Interface using imGUI
	- Load scenes
