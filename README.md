## AnSciVis-android
Raycaster for Android based on [ExSciVis](https://github.com/0x0AF/ExSciVis) framework built using glm, SDL2.0, imgui and stb-image.
Rendering and interaction specifics were tweaked, debugged and optimized for mobile in [AnSciVis](https://github.com/0x0AF/AnSciVis).

### GLES 3.0
All shaders altered to comply with Open GL ES 3.0 profile. ```mediump``` precision used for floats and texture sampling. To compensate for unavailability of ```GL_ALPHA_TEST``` fragment shader was updated to discard (nearly-) transparent fragments.

### Mobile optimization
GUI was optimized for touchscreen (increased sizes/paddings/margins). Shaders and data loaded through Android/SDL asset management pipeline (using SDL_rwops.h).

### Target aplication binary interface
App is deployed for ```armeabi-v7a``` to optimize compilation time. Coverage could be expanded to all ABIs.

### Target API level
App is targeted at API level 23. Can target [starting from 21](https://developer.android.com/guide/topics/graphics/opengl.html) with no issues.

### Screenshots<sup>1</sup>
<sup>[1]: Captured with Nexus7</sup>

![Screenshot 0](/scrn-0.png?raw=true "Screenshot 0")
![Screenshot 1](/scrn-1.png?raw=true "Screenshot 1")
![Screenshot 2](/scrn-2.png?raw=true "Screenshot 2")
![Screenshot 3](/scrn-3.png?raw=true "Screenshot 3")
![Screenshot 4](/scrn-4.png?raw=true "Screenshot 4")
