# Development Goals
This file details some goals and ideas for the project.

---

## Adding documentation
It would be nice to have some extra documentation. A lot of function parameters have
cryptic names that might not be obvious a someone unfamiliar with the project.

When I started working on this project, I felt completely lost and it took me a good while
to finally get the hang of things. I don't want others to have to deal with that.

---

## Moving configuration data to a file
While using registry keys is nice and all, the settings don't port over between
versions and that's an issue. It also makes it easier to edit if you know what
you are doing.

Since we can already parse XML consider using that.

---

## Simplifying the way BSA archives are identified
It took 20 minutes to launch Nif Skope when I had my settings cleared because of
the version shenanigans. This is caused by the large amount of files in my game directories.

---

## Making it so that you can select BSConnectionPoints in the 3d view.
Adding some functionality to select workshop connection points would be nice.
The goal is to have them show up when enable marker rendering.

It should also be mentioned that the assets don't need the editor markers that are bundled in production files.
Those are for the eyes of the devs only and we don't have to care about them.

---

## Add the ability to move/rotate/scale things in the 3d view.
IDK how to do this, but I would love to be able to use the 3d view to move things
instead of having to edit the values in the block details.

Honestly it might just be more efficient to work on [PyNifly](https://github.com/BadDogSkyrim/PyNifly)
for that. The blender nif tools don't seem to ever work for me and yeah.

An other option would be essentially migrating niftools to become an an actual blender plugin.
The issue is that i have no idea on how to get started with so it's probably not happening for a while.

Notes for that:
- Isolate nif stuff to a seperate tab in the object properties instead of using custom props.
  [blender_mmd_tools](https://github.com/UuuNyaa/blender_mmd_tools/releases/tag/v2.8.0) does that already.
- Create an automatic tool to create python classes from NifXml.


---

## Add visualization of bhkPhysicsSystem
compiled bhkPhysicsSystem have been documented and we should be able to parse them.

This projects seems to have figured it out https://github.com/Dexesttp/hkxpack.
It can transform the packed binary data into xml and therefore probably can parse it.

---

## Physics validation for to prepare for Elric
for a bhkCollisionObject, it appears as though the block number must go in decreasing order as you go down the tree.
Elric crashes when you provide it anything else.
eg:
```
5 bhkCollisionObject
	4 bhkRigidBodyT
		3 bhkBoxShape
```

---

## Fix .OBJ importing when no material is present.
When loading an obj without a material it just fails to import the whole thing. It should display a warning and add default values. 

---

## Add the option to replace mesh with obj.
If the current block has entries for mesh data we could give the option to replace that mesh the data from a .obj file.

---

## Prepare for QT6.
I'm likely going to have to upgrade the project to QT6 at some point so getting rid of stuff that was depreciated in 6.0 should be a focus.