#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <utility>
#include <thread>
#include "wavetable.h"
#include "imgui_includes.h"
#include "Synth.h"
#include <map>

void glfw_error_callback(int error, const char* description){
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int main(int, char**) {
    int display_w, display_h;

    // start setting up glfw
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, 1);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Lookup-table Synthesizer", nullptr, nullptr);
    if (window == nullptr)
        return 1;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    Synth st;
    ScopedPaHandler paInit;
    
    // Check that port audio streams are opened correctly with no errors
    if (paInit.result()) 
    {
        fprintf(stderr, "An error occurred while using the portaudio stream\n");
        fprintf(stderr, "Error number: %d\n", paInit.result());
        fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(paInit.result()));
        return 1;
    }
    if (!st.open(Pa_GetDefaultOutputDevice())) 
    {
        fprintf(stderr, "An error occurred while using the portaudio stream\n");
        return 1;
    }
    if (!st.start()) 
    {
        fprintf(stderr, "An error occurred while using the portaudio stream\n");
        return 1;
    }

    // Start ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // enable keyboard 
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // enable gamepad 

    // Set up for glfw + OpenGL
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Default window flags for use on all windows
    const bool no_titlebar            = false;
    const bool no_scrollbar           = false;
    const bool no_menu                = true;
    const bool no_move                = false;
    const bool no_resize              = false;
    const bool no_collapse            = false;
    const bool no_nav                 = false;
    const bool no_background          = false;
    const bool no_bring_to_front      = false;
    const bool unsaved_document       = false;

    // transparent background color
    ImVec4 clear_color = ImVec4(0.20f, 0.09f, 0.14f, 0.95f); 

    // apply window flags
    ImGuiWindowFlags window_flags = 0;
    if (no_titlebar)        window_flags |= ImGuiWindowFlags_NoTitleBar;
    if (no_scrollbar)       window_flags |= ImGuiWindowFlags_NoScrollbar;
    if (!no_menu)           window_flags |= ImGuiWindowFlags_MenuBar;
    if (no_move)            window_flags |= ImGuiWindowFlags_NoMove;
    if (no_resize)          window_flags |= ImGuiWindowFlags_NoResize;
    if (no_collapse)        window_flags |= ImGuiWindowFlags_NoCollapse;
    if (no_nav)             window_flags |= ImGuiWindowFlags_NoNav;
    if (no_background)      window_flags |= ImGuiWindowFlags_NoBackground;
    if (no_bring_to_front)  window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
    if (unsaved_document)   window_flags |= ImGuiWindowFlags_UnsavedDocument;

    // wwaveform names for dropdown lists
    const char* waveforms[] = { "Sawtooth", "Sine", "Square", "Triangle", "Silence"};

    // notes for dropdown list, index used for freq manipulation
    const char* notes[] = { "A0", "A#0", "B0",
        "C1", "C#1", "D1", "D#1", "E1", "F1", "F#1", "G1", "G#1", "A1", "A#1", "B1",
        "C2", "C#2", "D2", "D#2", "E2", "F2", "F#2", "G2", "G#2", "A2", "A#2", "B2",
        "C3", "C#3", "D3", "D#3", "E3", "F3", "F#3", "G3", "G#3", "A3", "A#3", "B3",
        "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4", "A#4", "B4",
        "C5", "C#5", "D5", "D#5", "E5", "F5", "F#5", "G5", "G#5", "A5", "A#5", "B5" };

    // List of keys for keyboard-controlled pitch
    std::vector<ImGuiKey> keys = 
    {
        ImGuiKey_Z, 
        ImGuiKey_S,
        ImGuiKey_X,
        ImGuiKey_D,
        ImGuiKey_C,
        ImGuiKey_V,
        ImGuiKey_G,
        ImGuiKey_B,
        ImGuiKey_H,
        ImGuiKey_N,
        ImGuiKey_J,
        ImGuiKey_M,
        ImGuiKey_Comma
    };

    // Create a map of keys to musical notes at the base octave
    std::map<ImGuiKey, float> key_freqs;
    int i = 0;
    for (const auto& key : keys )
    {
        key_freqs[key] = std::pow(2, (float)((i + 3) / 12.0));
        i++;
    }

    // Multiplier for octave
    int base = 1;

    SetupImGuiStyle();
    while (!glfwWindowShouldClose(window))
    {
        // get ready for drawing GUI
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

        bool gui_updated = true;
        bool imgui_visible = true;

        size_t osc_idx = 0;
        for (auto& osc : st.oscs) 
        {
            ImGui::PushID(osc_idx);
            ImGui::Begin((std::string("Oscillator ") + std::string(1, osc->label)).c_str(), &imgui_visible, window_flags);
            ImGui::PlotLines("Waveform", (float*)osc->table, TABLE_SIZE, 0, nullptr, -1.1f, 1.1f, ImVec2(100.0f, 100.0f));
            ImGui::SeparatorText("Waveform");
            if (ImGui::Combo("Waveform", (int*)&osc->current_waveform, waveforms, IM_ARRAYSIZE(waveforms)))
                gui_updated = true;

            switch (osc->current_waveform) 
            {
                case 0: // saw not special 
                    gen_saw_wave(osc);
                    break;
                case 1: // sin not special
                    gen_sin_wave(osc);
                    break;
                case 2: // square has a pulse width
                    gen_sqr_wave(osc);
                    if (ImGui::CollapsingHeader("Square Settings", ImGuiTreeNodeFlags_DefaultOpen))
                        if (ImGui::DragFloat("Pulse Width", &osc->pulse_width, 0.0025f, 0.0f, 1.0f))
                            gui_updated = true;
                    break;
                case 3:
                    gen_tri_wave(osc, osc->pulse_width);
                    if (ImGui::CollapsingHeader("Triangle Settings", ImGuiTreeNodeFlags_DefaultOpen))
                        if (ImGui::DragFloat("Duty Cycle", &osc->pulse_width, 0.0025f, 0.0f, 1.0f))
                            gui_updated = true;
                    break;
                case 4:
                    gen_silence(osc);
            }

            // settings such as per channel pitch
            st.amplitude = 0.5;

            for (ImGuiKey key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key = (ImGuiKey)(key + 1)) 
            { 
                if (ImGui::IsKeyDown(key) && std::find(keys.begin(), keys.end(), key) != keys.end())
                {
                    osc->env.key_on(get_time());
                    // ImGui::Text((key < ImGuiKey_NamedKey_BEGIN) ? "\"%s\"" : "\"%s\" %d", ImGui::GetKeyName(key), key); 
                    osc->left_phase_inc = base * key_freqs[key];
                    osc->right_phase_inc = base * key_freqs[key];
                    osc->env.lock = true;
                }
                if (ImGui::IsKeyReleased(key))
                {
                    osc->env.keyoff_amp = osc->env.get_amp(get_time());
                    osc->env.key_off(get_time());
                    osc->env.lock = false;
                }
                
            }
            ImGui::SeparatorText("BASE");
            ImGui::Text("Base %d", base);
            ImGui::Text("Time %d", get_time());
            ImGui::Text("Note on %d", osc->env.note_on);
            ImGui::Text("Amp %f", osc->env.get_amp(get_time()));

            if (ImGui::BeginTable("ADSR Envelope", 5))
            {
                ImGui::TableNextColumn();
                ImGui::VSliderFloat("##A", {50.0f, 150.0f}, &osc->env.attack_time, 0.0f, 2500.0f);
                ImGui::TableNextColumn();
                ImGui::VSliderFloat("##D", {50.0f, 150.0f}, &osc->env.decay_time, 0.0f, 2500.0f);
                ImGui::TableNextColumn();
                ImGui::VSliderFloat("##S", {50.0f, 150.0f}, &osc->env.sustain_amp, 0.0f, 1.0f);
                ImGui::TableNextColumn();
                ImGui::VSliderFloat("##R", {50.0f, 150.0f}, &osc->env.release_time, 0.01f, 2500.0f);
                ImGui::TableNextColumn();
                ImGui::VSliderFloat("##Z", {50.0f, 150.0f}, &osc->amp, 0.0f, 1.0f);
                ImGui::EndTable();
            }

            ImGui::End();
            ++osc_idx;
            ImGui::PopID();
        }

        if (ImGui::IsKeyPressed(ImGuiKey_LeftShift, false))
            base = (base > 1) ? base / 2 : base; 
        if (ImGui::IsKeyPressed(ImGuiKey_RightShift))
            base = (base < 64) ? base * 2 : base; 

        // render all our shit 
        ImGui::Render();

        // set the window up for drawing
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        // draw
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    st.close();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return paNoError;
}
