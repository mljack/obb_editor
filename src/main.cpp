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
#include "material.h"
#include "render_item.h"
#include "texture.h"
#include <array>

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

int g_window_width = 1600;
int g_window_height = 900;
int g_image_width = 0;
int g_image_height = 0;

float g_image_x = -1;
float g_image_y = -1;

bool g_image_dragging = false;
float g_offset_x = 0.0;
float g_offset_y = 0.0;
float g_offset_dx = 0.0;
float g_offset_dy = 0.0;
float g_drag_x = -1;
float g_drag_y = -1;
float g_scale = 1.0;

bool g_marker_dragging_ready = false;
bool g_marker_rotating_ready = false;
bool g_marker_front_edge_ready = false;
bool g_marker_back_edge_ready = false;
bool g_marker_left_edge_ready = false;
bool g_marker_right_edge_ready = false;
bool g_marker_dragging = false;
bool g_marker_rotating = false;
float g_marker_offset_dx = 0.0;
float g_marker_offset_dy = 0.0;
float g_marker_offset_heading = 0.0;
float g_marker_drag_x = -1;
float g_marker_drag_y = -1;

struct Marker {
public:
    float x = 0.0f;
    float y = 0.0f;
    float length = 100.0f;
    float width = 50.0f;
    float heading = 15.0f;
};

Marker* g_latest_marker = new Marker;

void handle_mouse_down_event(const SDL_Event& e) {
    g_image_x = (e.button.x - g_offset_x - g_offset_dx) / g_scale;
    g_image_y = (e.button.y + g_offset_y + g_offset_dy - g_window_height) / g_scale + g_image_height;

    if (e.button.button == SDL_BUTTON_LEFT) {
        g_drag_x = e.button.x;
        g_drag_y = e.button.y;
        g_image_dragging = true;
    } else if (e.button.button == SDL_BUTTON_RIGHT) {
        g_marker_drag_x = g_image_x;
        g_marker_drag_y = g_image_y;
        if (g_marker_dragging_ready)
            g_marker_dragging = true;
        else if (g_marker_rotating_ready)
            g_marker_rotating = true;
        else {
            glm::vec2 c(g_latest_marker->x, g_latest_marker->y);
            glm::vec2 v(g_image_x, g_image_y);
            v -= c;
            float yaw = glm::radians(g_latest_marker->heading);
            glm::vec2 dir(std::cos(yaw), std::sin(yaw));
            glm::vec2 normal(-std::sin(yaw), std::cos(yaw));
            float s = dot(v, dir);
            float l = dot(v, normal);
            if (g_marker_front_edge_ready) {
                c += (s - g_latest_marker->length / 2) / 2 * dir;
                g_latest_marker->length = s + g_latest_marker->length / 2;
            } else if (g_marker_back_edge_ready) {
                c += (s + g_latest_marker->length / 2) / 2 * dir;
                g_latest_marker->length = -s + g_latest_marker->length / 2;
            } else if (g_marker_left_edge_ready) {
                c += (l + g_latest_marker->width / 2) / 2 * normal;
                g_latest_marker->width = -l + g_latest_marker->width / 2;
            } else if (g_marker_right_edge_ready) {
                c += (l - g_latest_marker->width / 2) / 2 * normal;
                g_latest_marker->width = l + g_latest_marker->width / 2;
            }
            g_latest_marker->x = c.x;
            g_latest_marker->y = c.y;
        }
        //printf("marker: [%.1f, %.1f, %.1f, %.1f, %.1f]\n", g_latest_marker->x, g_latest_marker->y, g_latest_marker->length, g_latest_marker->width, g_latest_marker->heading);
    }
    //printf("mouse: [%.1f, %.1f]\n", g_image_x, g_image_y);
}

void handle_mouse_up_event(const SDL_Event& e) {
    if (e.button.button == SDL_BUTTON_LEFT) {
        g_offset_x += g_offset_dx;
        g_offset_y += g_offset_dy;
        g_offset_dx = 0;
        g_offset_dy = 0;
        g_drag_x = -1;
        g_drag_y = -1;
        g_image_dragging = false;
    } else if (e.button.button == SDL_BUTTON_RIGHT) {
        g_latest_marker->x += g_marker_offset_dx;
        g_latest_marker->y += g_marker_offset_dy;
        g_latest_marker->heading = std::fmod(g_latest_marker->heading + g_marker_offset_heading, 360.0f);
        g_marker_offset_dx = 0.0;
        g_marker_offset_dy = 0.0;
        g_marker_offset_heading = 0.0;
        g_marker_drag_x = -1;
        g_marker_drag_y = -1;
        g_marker_dragging = false;
        g_marker_rotating = false;
        //printf("marker: [%.1f, %.1f, %.1f, %.1f, %.1f]\n", g_latest_marker->x, g_latest_marker->y, g_latest_marker->length, g_latest_marker->width, g_latest_marker->heading);
    }
}

void handle_mouse_move_event(const SDL_Event& e) {
    //printf("mouse: [%d, %d]\n", e.motion.x, e.motion.y);
    if (g_image_dragging) {
        g_offset_dx = e.motion.x - g_drag_x;
        g_offset_dy = -(e.motion.y - g_drag_y);
    }
    //printf("g_offset: [%.1f, %.1f]\n", g_offset_x, g_offset_y);
    g_image_x = (e.motion.x - g_offset_x - g_offset_dx) / g_scale;
    g_image_y = (e.motion.y + g_offset_y + g_offset_dy - g_window_height) / g_scale + g_image_height;
    //printf("image: [%.1f, %.1f]\n", g_image_x, g_image_y);

    if (g_marker_dragging) {
        g_marker_offset_dx = g_image_x - g_marker_drag_x;
        g_marker_offset_dy = g_image_y - g_marker_drag_y;
        //printf("marker dxdy: [%.1f, %.1f]\n", g_marker_offset_dx, g_marker_offset_dy);
    } else if (g_marker_rotating) {
        g_marker_offset_heading = (g_marker_drag_x - g_image_x) * 0.3;
    } else {
        glm::vec2 v(g_image_x - g_latest_marker->x, g_image_y - g_latest_marker->y);
        float yaw = glm::radians(g_latest_marker->heading);
        glm::vec2 dir(std::cos(yaw), std::sin(yaw));
        glm::vec2 normal(-std::sin(yaw), std::cos(yaw));
        float s = dot(v, dir);
        float l = dot(v, normal);
        float length = g_latest_marker->length / 2;
        float width = g_latest_marker->width / 2;
        float r = std::sqrt(s * s + l * l);
        float R = std::sqrt(width * width + length * length);
        //printf("marker dist: [%.1f, %.1f]\n", s, l);
        float ratio = std::abs(s / l);
        float marker_ratio = length / width;
        float drag_radius = width / 2;
        g_marker_dragging_ready = (r <= drag_radius);
        g_marker_front_edge_ready = (r > drag_radius && s > 0 && ((s < length && ratio > marker_ratio) || (s >= length && std::abs(l) < width && s < R * 3)));
        g_marker_back_edge_ready = (r > drag_radius && s < 0 && ((s > -length && ratio > marker_ratio) || (s <= -length && std::abs(l) < width && s > -R * 3)));
        g_marker_left_edge_ready = (r > drag_radius && l < 0 && ((l >- width && ratio < marker_ratio) || (l <= -width && std::abs(s) < length && l > -width * 2)));
        g_marker_right_edge_ready = (r > drag_radius && l > 0 && ((l < width && ratio < marker_ratio) || (l >= width && std::abs(s) < length && l < width * 2)));
        g_marker_rotating_ready = !(g_marker_dragging_ready || g_marker_front_edge_ready || g_marker_back_edge_ready || g_marker_left_edge_ready || g_marker_right_edge_ready);
    }
}

void handle_mouse_wheel_event(const SDL_Event& e) {
#if defined(__EMSCRIPTEN__)
    //printf("wheel [%d][%d][%d, %d][%f, %f]\n", e.wheel.timestamp, e.wheel.direction, e.wheel.x, e.wheel.y, e.wheel.preciseX, e.wheel.preciseY);
    bool up = e.wheel.preciseY > 0;
#else
    bool up = e.wheel.y > 0;
#endif

    float s = 1.2f;
    if (!up)
        s = 1.0f / s;

    g_offset_x += g_image_x * g_scale * (1 - s) ;
    g_offset_y += (g_image_height - g_image_y) * g_scale * (1 - s) ;
    g_scale = std::min(200.0f, std::max(1.0f/200.0f, g_scale*s));
}

Material* g_background_img_material = nullptr;
Material* g_line_material = nullptr;

void clean_up() {
    delete g_background_img_material;
    delete g_line_material;
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


#if defined(__EMSCRIPTEN__)
EM_JS(int, canvas_get_width, (), {
return canvas.width;
});

EM_JS(int, canvas_get_height, (), {
return canvas.height;
});
#endif

void build_marker_geom(std::vector<GLfloat>* v_buf, std::vector<GLuint>* idx_buf) {
    float yaw = glm::radians(g_latest_marker->heading + g_marker_offset_heading);
    glm::vec2 c(g_latest_marker->x + g_marker_offset_dx, g_latest_marker->y + g_marker_offset_dy);
    glm::vec2 dir(std::cos(yaw), std::sin(yaw));
    glm::vec2 normal(-std::sin(yaw), std::cos(yaw));

    float length = g_latest_marker->length / 2;
    float width = g_latest_marker->width / 2;
    std::array<glm::vec2, 16> pts = {
        c - length * dir - width * normal,
        c + length * dir - width * normal,
        c + length * dir - width * normal,
        c + length * dir + width * normal,
        c + length * dir + width * normal,
        c - length * dir + width * normal,
        c - length * dir + width * normal,
        c - length * dir - width * normal,
        c + length * dir,
        c + (length + width / 2) * dir,
        c + (length + width / 4) * dir + width / 4 * normal,
        c + (length + width / 4) * dir - width / 4 * normal,
        c - glm::vec2(width / 4, 0.0f),
        c + glm::vec2(width / 4, 0.0f),
        c - glm::vec2(0.0f, width / 4),
        c + glm::vec2(0.0f, width / 4),
    };
    GLuint base_idx = (GLuint)v_buf->size() / 7;
    for(auto& pt : pts) {
        v_buf->push_back(pt.x);
        v_buf->push_back(g_image_height - pt.y);
        v_buf->push_back(10.0f);
        auto idx = &pt-pts.data();
        bool ready = false;
        ready = ready || (g_marker_dragging_ready && idx >=12 && idx <= 15);
        ready = ready || (g_marker_front_edge_ready && idx >=2 && idx <= 3);
        ready = ready || (g_marker_back_edge_ready && idx >= 6 && idx <= 7);
        ready = ready || (g_marker_left_edge_ready && idx >= 0 && idx <= 1);
        ready = ready || (g_marker_right_edge_ready && idx >= 4 && idx <= 5);
        ready = ready || (g_marker_rotating_ready && idx >= 8 && idx <= 11);
        if (ready) {
            v_buf->push_back(0.0f);
            v_buf->push_back(0.0f);
            v_buf->push_back(1.0f);
            v_buf->push_back(1.0f);
        } else {
            v_buf->push_back(1.0f);
            v_buf->push_back(0.0f);
            v_buf->push_back(0.0f);
            v_buf->push_back(1.0f);
        }
    }
    idx_buf->push_back(base_idx + 0); idx_buf->push_back(base_idx + 1);
    idx_buf->push_back(base_idx + 2); idx_buf->push_back(base_idx + 3);
    idx_buf->push_back(base_idx + 4); idx_buf->push_back(base_idx + 5);
    idx_buf->push_back(base_idx + 6); idx_buf->push_back(base_idx + 7);
    //idx_buf->push_back(base_idx + 5); idx_buf->push_back(base_idx + 1);
    //idx_buf->push_back(base_idx + 5); idx_buf->push_back(base_idx + 2);
    idx_buf->push_back(base_idx + 8); idx_buf->push_back(base_idx + 9);
    idx_buf->push_back(base_idx + 9); idx_buf->push_back(base_idx + 10);
    idx_buf->push_back(base_idx + 9); idx_buf->push_back(base_idx + 11);
    idx_buf->push_back(base_idx + 12); idx_buf->push_back(base_idx + 13);
    idx_buf->push_back(base_idx + 14); idx_buf->push_back(base_idx + 15);
}

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

#if defined(__EMSCRIPTEN__)
    g_window_width = canvas_get_width();
    g_window_height = canvas_get_height();
#endif
    printf("window: [%d, %d]\n", g_window_width, g_window_height);

    g_background_img_material = new Material;
    if (!g_background_img_material->build(vertex_shader_src, fragment_shader_src))
        return -1;
    g_line_material = new Material;
    if (!g_line_material->build(vertex_shader_src2, fragment_shader_src2))
        return -1;

#if 0
#define BACKGROUND_IMG "cat.jpg"
#else
#define BACKGROUND_IMG "6k.png"
#endif

#if defined(__EMSCRIPTEN__)
    unsigned int texture_id = load_texture("/resources/" BACKGROUND_IMG, &g_image_width, &g_image_height);
#else
    unsigned int texture_id = load_texture("../resources/" BACKGROUND_IMG, &g_image_width, &g_image_height);
#endif

    g_scale = std::min(g_window_width / (float)g_image_width, g_window_height / (float)g_image_height);
    g_offset_x = (g_window_width - g_image_width * g_scale) / 2;
    g_offset_y = (g_window_height - g_image_height * g_scale) / 2;

    g_latest_marker->x = g_image_width * 0.5f;
    g_latest_marker->y = g_image_height * 0.5f;

    const float base_x = 0.0f;
    const float base_y = 0.0f;
    const float w = g_image_width;
    const float h = g_image_height;
    std::vector<GLfloat> vertices2 = {
        // x        y        z     r     g     b     a     u     v
        base_x  , base_y  , 0.0f, 1.0f, 1.0f, 1.0f, 0.5f, 0.0f, 1.0f,
        base_x+w, base_y  , 0.0f, 1.0f, 1.0f, 1.0f, 0.5f, 1.0f, 1.0f,
        base_x+w, base_y+h, 0.0f, 1.0f, 1.0f, 1.0f, 0.5f, 1.0f, 0.0f,
        base_x  , base_y+h, 0.0f, 1.0f, 1.0f, 1.0f, 0.5f, 0.0f, 0.0f,
    };
    std::vector<GLuint> indices2 = {
        0, 1, 2, 0, 2, 3
    };

    RenderItem rect(GL_TRIANGLES);
    rect.set_material(g_background_img_material);
    rect.update_buffers_for_textured_geoms(vertices2, indices2);

    RenderItem lines(GL_LINES);
    lines.set_material(g_line_material);


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
            else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
                done = true;

            if (!io.WantCaptureMouse) {
                if (event.type == SDL_MOUSEBUTTONDOWN)
                    handle_mouse_down_event(event);
                else if (event.type == SDL_MOUSEBUTTONUP)
                    handle_mouse_up_event(event);
                else if (event.type == SDL_MOUSEWHEEL)
                    handle_mouse_wheel_event(event);
                else if (event.type == SDL_MOUSEMOTION)
                    handle_mouse_move_event(event);
            }
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
        view = view * glm::scale(model, glm::vec3(g_scale, g_scale, 1.0));
        glm::mat4 proj = glm::ortho(0.0f, (float)g_window_width, 0.0f, (float)g_window_height, -1000.0f, 1000.0f);
        {
            glm::mat4 xform = proj * view * model;
            glBindTexture(GL_TEXTURE_2D, texture_id);
            rect.render_textured(xform);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        {
            std::vector<GLfloat> vertices3;
            std::vector<GLuint> indices3;
            build_marker_geom(&vertices3, &indices3);
            lines.update_buffers_for_nontextured_geoms(vertices3, indices3);

            glm::mat4 xform = proj * view;
            glLineWidth(2.0f);
            lines.render_nontextured(xform);
            glLineWidth(1.0f);
        }

        // Rendering GUI
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
#if !defined(__EMSCRIPTEN__)
        SDL_Delay(17);
#endif
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
