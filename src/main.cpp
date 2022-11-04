#include <stdio.h>
#include <SDL.h>
#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#include <SDL_opengles2.h>
#elif defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else

#define GL_GLEXT_PROTOTYPES

#include <SDL_opengl.h>
//#include <SDL_opengl_glext.h>
#endif

#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <portable-file-dialogs.h>
#include "shaders.h"

#if defined(__EMSCRIPTEN__)
// Emscripten wants to run the mainloop because of the way the browser is single threaded.
// For this, it requires a void() function. In order to avoid breaking the flow of the
// readability of this example, we're going to use a C++11 lambda to keep the mainloop code
// within the rest of the main function. This hack isn't needed in production, and one may
// simply have an actual main loop function to give to emscripten instead.
#include <functional>
static std::function<void()> loop;
static void main_loop() { loop(); }
#endif

class Material {
public:
    bool build(const GLchar* vertexShaderSource, const GLchar* fragmentShaderSource) {
        // Build and compile our shader program
        // Vertex shader
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);
        // Check for compile time errors
        GLint success;
        GLchar infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED: %s\n", infoLog);
            return 0;
        }
        // Fragment shader
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);
        // Check for compile time errors
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED: %s\n", infoLog);
            return 0;
        }
        // Link shaders
        shader_prog_ = glCreateProgram();
        glAttachShader(shader_prog_, vertexShader);
        glAttachShader(shader_prog_, fragmentShader);
        glLinkProgram(shader_prog_);
        // Check for linking errors
        glGetProgramiv(shader_prog_, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader_prog_, 512, NULL, infoLog);
            printf("ERROR::PROGRAM::LINK_FAILED: %s\n", infoLog);
            return 0;
        }
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return true;
    }
    void use() {
        glUseProgram(shader_prog_);
    }
    void set_transform(const glm::mat4& xform) {
        unsigned int xform_uniform = glGetUniformLocation(shader_prog_, "xform");
        glUniformMatrix4fv(xform_uniform, 1, GL_FALSE, glm::value_ptr(xform));
    }
private:
    GLuint shader_prog_;
};

class RenderItem {
public:
    RenderItem(GLenum primitive_type) {
        primitive_type_ = primitive_type;
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        material_ = nullptr;
    }
    ~RenderItem() {
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }
    void update_buffers(const std::vector<GLfloat>& vertices, const std::vector<GLuint>& indices) {
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0])*vertices.size(), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0])*indices.size(), indices.data(), GL_STATIC_DRAW);

        // position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(0);

        // color attribute
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3* sizeof(float)));
        glEnableVertexAttribArray(1);

        // texture coord attribute
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(7 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        n_indices_ = (GLsizei)indices.size();
    }
    void render(const glm::mat4& xform) {
        assert(material_);

        material_->use();
        material_->set_transform(xform);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glDrawElements(primitive_type_, n_indices_, GL_UNSIGNED_INT, 0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    void set_material(Material* m) { material_ = m; }

private:
    GLenum primitive_type_;
    GLsizei n_indices_;
    GLuint VBO, EBO;
    Material *material_;
};

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;
float g_scale = 1.0;
float g_offset_x = 0.0;
float g_offset_y = 0.0;
float g_offset_dx = 0.0;
float g_offset_dy = 0.0;
int g_drag_x = -1;
int g_drag_y = -1;

void handle_mouse_down_event(const SDL_Event& e) {
    if (e.button.button == SDL_BUTTON_LEFT) {
        g_drag_x = e.button.x;
        g_drag_y = e.button.y;
    } else if (e.button.button == SDL_BUTTON_RIGHT) {

    }
}

void handle_mouse_up_event(const SDL_Event& e) {
    if (e.button.button == SDL_BUTTON_LEFT) {
        g_offset_x += g_offset_dx;
        g_offset_y += g_offset_dy;
        g_offset_dx = 0;
        g_offset_dy = 0;
        g_drag_x = -1;
        g_drag_y = -1;
    } else if (e.button.button == SDL_BUTTON_RIGHT) {

    }
}

void handle_mouse_move_event(const SDL_Event& e) {
    printf("mouse: [%d, %d]\n", e.motion.x, e.motion.y);
    if (g_drag_x >= 0) {
        g_offset_dx = e.button.x - g_drag_x;
        g_offset_dy = -(e.button.y - g_drag_y);
    }
}

void handle_mouse_wheel_event(const SDL_Event& e) {
#if defined(__EMSCRIPTEN__)
    printf("wheel [%d][%d][%d, %d][%f, %f]\n", e.wheel.timestamp, e.wheel.direction, e.wheel.x, e.wheel.y, e.wheel.preciseX, e.wheel.preciseY);
    bool up = e.wheel.preciseY > 0;
#else
    bool up = e.wheel.y > 0;
#endif

    float s = 1.05f;
    if (!up)
        s = 1.0f / s;
    g_scale = std::min(200.0f, std::max(1.0f/200.0f, g_scale*s));
}

unsigned int load_texture(const char* file_path) {
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int width, height, nrChannels;
    printf("[%s]\n", file_path);
    unsigned char *data = stbi_load(file_path, &width, &height, &nrChannels, 0);
    if (data) {
        printf("[%d x %d], %d\n", width, height, nrChannels);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);
    return texture;
}

Material* g_material = nullptr;

void clean_up() {
    delete g_material;
    // Terminate GLFW, clearing any resources allocated by GLFW.
    //glfwTerminate();
    //OUT std::cout << "exit main" << std::endl;
}

//void accumulate_buffers(double base_x, double base_y, const Mesh3D& mesh, std::vector<GLfloat>* vertices, std::vector<GLuint>* indices) {
//    double scale = 200.0;
//    vertices->reserve(mesh.vertices.size()*3 + vertices->size());
//    indices->reserve(mesh.indices.size() + indices->size());
//    GLuint base_idx = (GLuint)vertices->size() / 3;
//    for (auto& v : mesh.vertices) {
//        vertices->push_back((GLfloat)((v[0]-base_x)/scale));
//        vertices->push_back((GLfloat)((v[1]-base_y)/scale));
//        vertices->push_back((GLfloat)v[2]);
//    }
//    for (auto idx : mesh.indices)
//        indices->push_back((GLuint)(base_idx + idx));
//}

// Main code
int main(int, char**)
{
    // Setup SDL
    // (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
    // depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to the latest version of SDL is recommended!)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Decide GL+GLSL versions
#if defined(__EMSCRIPTEN__)
    // For the browser using Emscripten, we are going to use WebGL1 with GL ES2. See the Makefile. for requirement details.
    // It is very likely the generated file won't work in many browsers. Firefox is the only sure bet, but I have successfully
    // run this code on Chrome for Android for example.
    // GL ES 3.0 + GLSL 300 ES (WebGL2)
    const char* glsl_version = "#version 300 es";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("obb_editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    g_material = new Material;
    if (!g_material->build(vertexShaderSource, fragmentShaderSource))
        return -1;

    const float base_x = 0.0f;
    const float base_y = 0.0f;
    const float w = 800.0f;
    const float h = 800.0f;
    std::vector<GLfloat> vertices2 = {
        // x        y        z     r     g     b     a     u     v
        base_x  , base_y  , 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        base_x+w, base_y  , 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f,
        base_x+w, base_y+h, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
        base_x  , base_y+h, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
    };
    std::vector<GLuint> indices2 = {
        0, 1, 2, 0, 2, 3
    };
    
    RenderItem rect(GL_TRIANGLES);
    rect.set_material(g_material);
    rect.update_buffers(vertices2, indices2);

#if defined(__EMSCRIPTEN__)
    unsigned int texture_id = load_texture("/resources/cat.jpg");
#else
    unsigned int texture_id = load_texture("../resources/cat.jpg");
#endif

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
#if defined(__EMSCRIPTEN__)
    // See comments around line 30.
    loop = [&]()
    // Note that this doesn't process the 'done' boolean anymore. The function emscripten_set_main_loop will
    // in effect not return, and will set an infinite loop, that is going to be impossible to break. The
    // application can then only stop when the brower's tab is closed, and you will NOT get any notification
    // about exitting. The browser will then cleanup all resources on its own.
#else
    while (!done)
#endif
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
            else if (event.type == SDL_MOUSEBUTTONDOWN)
                handle_mouse_down_event(event);
            else if (event.type == SDL_MOUSEBUTTONUP)
                handle_mouse_up_event(event);
            else if (event.type == SDL_MOUSEWHEEL)
                handle_mouse_wheel_event(event);
            else if (event.type == SDL_MOUSEMOTION)
                handle_mouse_move_event(event);
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window) {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // Rendering Viewport
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        //printf("[%d x %d]\n", (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        glm::mat4 model(1.0);
        glm::mat4 view(1.0);
        //model = glm::rotate(model, glm::radians(-55.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        view = glm::translate(view, glm::vec3(g_offset_x + g_offset_dx, g_offset_y + g_offset_dy, 0.0f));
        model = glm::scale(model, glm::vec3(g_scale, g_scale, 1.0));
        glm::mat4 proj = glm::ortho(0.0f, (float)WINDOW_WIDTH, 0.0f, (float)WINDOW_HEIGHT, -1000.0f, 1000.0f);
        //glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 1000.0f);


        glBindTexture(GL_TEXTURE_2D, texture_id);
        glm::mat4 xform = proj * view * model;
        rect.render(xform);

        // Rendering GUI
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    };
#if defined(__EMSCRIPTEN__)
    // See comments around line 30.
    // This function call will not return.
    emscripten_set_main_loop(main_loop, 0, true);
    // A fully portable version of this code that doesn't use the lambda hack might have an #else that does:
    //    while (!done) main_loop();
#endif

    clean_up();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
