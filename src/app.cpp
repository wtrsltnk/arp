
#include <algorithm>
#include <app.hpp>
#include <chrono>
#include <glad/glad.h>
#include <imgui.h>
#include <sstream>

#include "imgui_knob.h"

const int Note_C_OffsetFromC = 0;
const int Note_CSharp_OffsetFromC = 1;
const int Note_D_OffsetFromC = 2;
const int Note_DSharp_OffsetFromC = 3;
const int Note_E_OffsetFromC = 4;
const int Note_F_OffsetFromC = 5;
const int Note_FSharp_OffsetFromC = 6;
const int Note_G_OffsetFromC = 7;
const int Note_GSharp_OffsetFromC = 8;
const int Note_A_OffsetFromC = 9;
const int Note_ASharp_OffsetFromC = 10;
const int Note_B_OffsetFromC = 11;
const int firstKeyNoteNumber = 24;

void App::OnInit()
{
    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 22.0f);

    auto &style = ImGui::GetStyle();
    style.ItemSpacing = ImVec2(10, 10);

    glClearColor(0.56f, 0.7f, 0.67f, 1.0f);

    // RtMidiOut constructor
    try
    {
        _midiout = new RtMidiOut();
    }
    catch (RtMidiError &error)
    {
        error.printMessage();
        exit(EXIT_FAILURE);
    }

    auto ports = _midiout->getPortCount();

    for (unsigned int i = 0; i < ports; i++)
    {
        try
        {
            _portNames.push_back(_midiout->getPortName(i));
        }
        catch (RtMidiError &error)
        {
            error.printMessage();
        }
    }

    tChannel channel;
    channel._name = "First Arp";
    _channels.push_back(channel);
}

void App::OnResize(
    int width,
    int height)
{
    _width = width;
    _height = height;

    glViewport(0, 0, width, height);

    std::cout << _width << "x" << _height << std::endl;
}

ImVec2 buttonSize(50, 80);

const unsigned char MIDI_NOTE_ON = 144;
const unsigned char MIDI_NOTE_OFF = 128;

void App::PianoKey(
    struct tChannel &ch,
    const char *label,
    int noteNumberInOctave,
    unsigned char velocity)
{
    unsigned char note = firstKeyNoteNumber + (ch._octaveShift * 12) + noteNumberInOctave;

    ImGui::Button(label, buttonSize);

    if (notesDown.find(note) == notesDown.end() && ImGui::IsItemClicked())
    {
        unsigned char state = MIDI_NOTE_ON | ch._channel;

        std::vector<unsigned char> message = {
            state,
            note,
            velocity,
        };
        _midiout->sendMessage(&message);
        notesDown.insert(note);
        if (recordMode)
        {
            ch._notesToArp.push_back(note);
        }
    }
    else if (notesDown.find(note) != notesDown.end() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        unsigned char state = MIDI_NOTE_OFF | ch._channel;

        std::vector<unsigned char> message = {
            state,
            note,
            0,
        };
        _midiout->sendMessage(&message);
        notesDown.erase(note);
    }
}

void App::RemoveChannel(
    struct tChannel &ch)
{
    _channelToRemove = &ch;
}

enum ArpModes
{
    Up = 0,
    Down = 1,
    Inclusive = 2,
    Exclusive = 3,
    Random = 4,
    Order = 5,
};

void App::RunNotes()
{
    int msBetweenNotes = int(60000.0f / _bpm);

    static std::chrono::time_point<std::chrono::steady_clock> lastNote = std::chrono::steady_clock::now();
    static unsigned char lastPlayerNote = 0;
    static int currentDirection = 1;

    auto now = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastNote);

    if (!pauseMode && !recordMode)
    {
        for (auto &ch : _channels)
        {
            if (ch._notesToArp.empty())
            {
                continue;
            }

            auto notes = std::vector<unsigned char>(ch._notesToArp);

            if (ch._arpMode != ArpModes::Order)
            {
                std::sort(notes.begin(), notes.end());
            }

            if (ch._noteLength > 1.0f) ch._noteLength = 1.0f;
            if (ch._noteLength <= 0.0f) ch._noteLength = 0.01f;

            if (lastPlayerNote != 0 && diff.count() > (msBetweenNotes * ch._noteLength))
            {
                unsigned char state = MIDI_NOTE_OFF | ch._channel;

                std::vector<unsigned char> message = {
                    state,
                    lastPlayerNote,
                    0,
                };
                _midiout->sendMessage(&message);
                lastPlayerNote = 0;
            }

            if (diff.count() > msBetweenNotes)
            {
                unsigned char state = MIDI_NOTE_ON | ch._channel;

                lastPlayerNote = notes[ch._currentNote];
                std::vector<unsigned char> message = {
                    state,
                    lastPlayerNote,
                    static_cast<unsigned char>(ch._velocity),
                };
                _midiout->sendMessage(&message);
                lastNote = now;

                if (ch._arpMode == ArpModes::Up || ch._arpMode == ArpModes::Order)
                {
                    ch._currentNote++;
                    if (ch._currentNote >= ch._notesToArp.size())
                    {
                        ch._currentNote = 0;
                    }
                }
                else if (ch._arpMode == ArpModes::Down)
                {
                    if (ch._currentNote == 0)
                    {
                        ch._currentNote = ch._notesToArp.size() - 1;
                    }
                    else
                    {
                        ch._currentNote--;
                    }
                }
                else if (ch._arpMode == ArpModes::Inclusive)
                {
                    if (currentDirection < 0 && ch._currentNote == 0)
                    {
                        currentDirection = 1;
                    }
                    else if (currentDirection > 0 && ch._currentNote + 1 >= ch._notesToArp.size())
                    {
                        currentDirection = -1;
                    }
                    else
                    {
                        ch._currentNote += currentDirection;
                    }
                }
                else if (ch._arpMode == ArpModes::Exclusive)
                {
                    if (currentDirection < 0 && ch._currentNote == 0)
                    {
                        currentDirection = 1;
                    }
                    else if (currentDirection > 0 && ch._currentNote + 1 >= ch._notesToArp.size())
                    {
                        currentDirection = -1;
                    }

                    ch._currentNote += currentDirection;
                }
                else if (ch._arpMode == ArpModes::Random)
                {
                    ch._currentNote = std::rand() % ch._notesToArp.size();
                }
            }
        }
    }
    else
    {
        lastNote = now;
    }
}

void App::OnFrame()
{
    RunNotes();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(22.0f / 255.0f, 85.0f / 255.0f, 147.0f / 255.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f / 255.0f, 99.0f / 255.0f, 177.0f / 255.0f, 1.0f));
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(float(_width), float(_height)));
    ImGui::Begin("Test", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

    ImVec2 toolBarButtonSize(100, 30);

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, toolBarButtonSize.y / 2.0f);

    if (!recordMode && !pauseMode)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(99.0f / 255.0f, 0.0f / 255.0f, 177.0f / 255.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(22.0f / 255.0f, 0.0f / 255.0f, 100.0f / 255.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(22.0f / 255.0f, 0.0f / 255.0f, 100.0f / 255.0f, 1.0f));
    }

    ImGui::Button("Play", toolBarButtonSize);

    if (!recordMode && !pauseMode)
    {
        ImGui::PopStyleColor(3);
    }
    if (ImGui::IsItemClicked())
    {
        pauseMode = !pauseMode;
        if (recordMode)
        {
            pauseMode = false;
        }
        recordMode = false;
        if (pauseMode)
        {
            for (auto &ch : _channels)
            {
                for (auto &note : ch._notesToArp)
                {
                    unsigned char state = MIDI_NOTE_OFF | ch._channel;

                    std::vector<unsigned char> message = {
                        state,
                        note,
                        0,
                    };
                    _midiout->sendMessage(&message);
                }
            }
        }
    }

    ImGui::SameLine();

    if (recordMode)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(99.0f / 255.0f, 0.0f / 255.0f, 177.0f / 255.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(22.0f / 255.0f, 0.0f / 255.0f, 100.0f / 255.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(22.0f / 255.0f, 0.0f / 255.0f, 100.0f / 255.0f, 1.0f));
    }

    ImGui::Button("Record", toolBarButtonSize);

    if (recordMode)
    {
        ImGui::PopStyleColor(3);
    }
    ImGui::PopStyleVar();

    if (ImGui::IsItemClicked())
    {
        recordMode = !recordMode;
        if (recordMode)
        {
            for (auto &ch : _channels)
            {
                for (auto &note : ch._notesToArp)
                {
                    unsigned char state = MIDI_NOTE_OFF | ch._channel;

                    std::vector<unsigned char> message = {
                        state,
                        note,
                        0,
                    };
                    _midiout->sendMessage(&message);
                }
                ch._notesToArp.clear();
                ch._currentNote = 0;
            }
        }
    }

    ImGui::SameLine();

    ImGui::SliderFloat("BPM", &_bpm, 80.0f, 180.0f);

    ImGui::Separator();

    ImGui::BeginGroup();
    ImGui::Text("Midi output");
    static int e = 0;
    if (ImGui::RadioButton("No Midi output", &e, 0) && _midiout->isPortOpen())
    {
        _midiout->closePort();
    }

    for (size_t i = 0; i < _portNames.size(); i++)
    {
        ImGui::SameLine();
        if (ImGui::RadioButton(_portNames[i].c_str(), &e, int(i + 1)))
        {
            if (_midiout->isPortOpen())
            {
                _midiout->closePort();
            }
            _midiout->openPort(int(i));
        }
    }
    ImGui::EndGroup();

    ImGui::Separator();

    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None | ImGuiTabBarFlags_AutoSelectNewTabs;
    if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags))
    {
        for (auto &ch : _channels)
        {
            if (ImGui::BeginTabItem(ch._name.c_str(), nullptr))
            {
                RenderChannel(ch);
                ImGui::EndTabItem();
            }
        }

        if (ImGui::BeginTabItem("+"))
        {
            static char buf[64] = {0};
            static const char *error = nullptr;
            ImGui::InputText("Name", buf, 64);

            if (ImGui::Button("Create"))
            {
                error = nullptr;
                struct tChannel channel;
                channel._name = buf;
                for (auto &ch : _channels)
                {
                    if (ch._name == channel._name)
                    {
                        error = "Name already exists";
                        break;
                    }
                }

                if (error == nullptr)
                {
                    _channels.push_back(channel);
                    memset(buf, 0, 64);
                }
            }

            if (error != nullptr)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "%s", error);
            }

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
    ImGui::PopStyleColor(2);

    if (_channelToRemove != nullptr)
    {
        for (auto channel = _channels.begin(); channel != _channels.end(); ++channel)
        {
            if (channel->_name == _channelToRemove->_name)
            {
                _channels.erase(channel);
                _channelToRemove = nullptr;
                break;
            }
        }
    }
}

void App::RenderChannel(
    struct tChannel &ch)
{
    ImGui::SetNextItemWidth(200);

    static ImGuiComboFlags flags = 0;

    const char *channels[] = {
        "Midi channel 1",
        "Midi channel 2",
        "Midi channel 3",
        "Midi channel 4",
        "Midi channel 5",
        "Midi channel 6",
        "Midi channel 7",
        "Midi channel 8",
        "Midi channel 9",
        "Midi channel 10",
        "Midi channel 11",
        "Midi channel 12",
        "Midi channel 13",
        "Midi channel 14",
        "Midi channel 15",
        "Midi channel 16",
    };

    const char *combo_label = channels[ch._channel];

    if (ImGui::BeginCombo("##Channel", combo_label, flags))
    {
        for (int i = 0; i < 16; i++)
        {
            const bool is_selected = (ch._channel == i);
            if (ImGui::Selectable(channels[i], is_selected))
            {
                ch._channel = i;
            }

            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();

    static char buf[64] = {0};
    if (ImGui::Button("Change name"))
    {
        strcpy_s(buf, 64, ch._name.c_str());
        ImGui::OpenPopup("Change the name");
    }

    ImGui::SameLine(ImGui::GetWindowWidth() - 55);

    if (ImGui::Button("x", ImVec2(30.0f, 0.0f)))
    {
        RemoveChannel(ch);
    }

    if (ImGui::BeginPopupModal("Change the name", nullptr, ImGuiWindowFlags_NoResize))
    {
        static const char *error = nullptr;
        ImGui::InputText("##Name", buf, 64);

        if (error != nullptr)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "%s", error);
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "");
        }

        if (ImGui::Button("OK", ImVec2(120, 0)))
        {
            error = nullptr;

            for (auto &subch : _channels)
            {
                if (&ch == &subch)
                {
                    continue;
                }
                if (subch._name == std::string(buf))
                {
                    error = "Name already exists";
                    break;
                }
            }

            if (error == nullptr)
            {
                ch._name = buf;
                memset(buf, 0, 64);
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::BeginGroup();
    ImGui::Text("Arp Mode");
    ImGui::RadioButton("Up", &(ch._arpMode), ArpModes::Up);

    ImGui::SameLine();

    ImGui::RadioButton("Down", &(ch._arpMode), ArpModes::Down);

    ImGui::SameLine();

    ImGui::RadioButton("Inclusive up/down", &(ch._arpMode), ArpModes::Inclusive);

    ImGui::SameLine();

    ImGui::RadioButton("Exclusive up/down", &(ch._arpMode), ArpModes::Exclusive);

    ImGui::SameLine();

    ImGui::RadioButton("Random", &(ch._arpMode), ArpModes::Random);

    ImGui::SameLine();

    ImGui::RadioButton("Order", &(ch._arpMode), ArpModes::Order);
    ImGui::EndGroup();

    ImGui::Separator();

    ImGui::KnobUchar("Velocity", &(ch._velocity), 0, 127, ImVec2(80.0f, 60.0f));

    ImGui::SameLine();

    ImGui::KnobInt("Octave shift", &(ch._octaveShift), 0, 8, ImVec2(80.0f, 60.0f));

    ImGui::SameLine();

    ImGui::Knob("Note length", &(ch._noteLength), 0.01f, 0.99f, ImVec2(80.0f, 60.0f));

    ImGui::SameLine();

    ImGui::BeginGroup();
    ImGui::Text("Operations on recorded notes");
    if (ImGui::Button("octave down"))
    {
        for (auto &note : ch._notesToArp)
        {
            note -= 12;
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("octave up"))
    {
        for (auto &note : ch._notesToArp)
        {
            note += 12;
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("note down"))
    {
        for (auto &note : ch._notesToArp)
        {
            note -= 1;
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("note up"))
    {
        for (auto &note : ch._notesToArp)
        {
            note += 1;
        }
    }

    ImGui::EndGroup();

    ImGui::Separator();

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, buttonSize.x / 2.0f);

    { // Top Row
        ImGui::InvisibleButton("##", ImVec2(buttonSize.x / 2.0f, buttonSize.y));

        ImGui::SameLine();

        PianoKey(ch, "C#", Note_CSharp_OffsetFromC, ch._velocity);

        ImGui::SameLine();

        PianoKey(ch, "D#", Note_DSharp_OffsetFromC, ch._velocity);

        ImGui::SameLine();

        ImGui::InvisibleButton("##", buttonSize);

        ImGui::SameLine();

        PianoKey(ch, "F#", Note_FSharp_OffsetFromC, ch._velocity);

        ImGui::SameLine();

        PianoKey(ch, "G#", Note_GSharp_OffsetFromC, ch._velocity);

        ImGui::SameLine();

        PianoKey(ch, "A#", Note_ASharp_OffsetFromC, ch._velocity);
    }

    { // Bottom Row

        PianoKey(ch, "C", Note_C_OffsetFromC, ch._velocity);

        ImGui::SameLine();

        PianoKey(ch, "D", Note_D_OffsetFromC, ch._velocity);

        ImGui::SameLine();

        PianoKey(ch, "E", Note_E_OffsetFromC, ch._velocity);

        ImGui::SameLine();

        PianoKey(ch, "F", Note_F_OffsetFromC, ch._velocity);

        ImGui::SameLine();

        PianoKey(ch, "G", Note_G_OffsetFromC, ch._velocity);

        ImGui::SameLine();

        PianoKey(ch, "A", Note_A_OffsetFromC, ch._velocity);

        ImGui::SameLine();

        PianoKey(ch, "B", Note_B_OffsetFromC, ch._velocity);
    }

    ImGui::PopStyleVar();
}

void App::OnExit()
{
    for (auto &ch : _channels)
    {
        for (auto &note : ch._notesToArp)
        {
            unsigned char state = MIDI_NOTE_OFF | ch._channel;

            std::vector<unsigned char> message = {
                state,
                note,
                0,
            };
            _midiout->sendMessage(&message);
        }
    }

    _midiout->closePort();
    delete _midiout;
    _midiout = nullptr;
}
