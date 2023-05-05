#ifndef APP_H
#define APP_H

#include <set>
#include <string>
#include <vector>

#include <RtMidi.h>

class App
{
public:
    App(const std::vector<std::string> &args);
    virtual ~App();

    bool Init();
    int Run();

    void OnInit();
    void OnFrame();
    void OnResize(int width, int height);
    void OnExit();

    template <class T>
    T *GetWindowHandle() const;

protected:
    const std::vector<std::string> &_args;
    int _width = 1024;
    int _height = 525;

    template <class T>
    void SetWindowHandle(T *handle);

    void ClearWindowHandle();

    RtMidiOut *_midiout = nullptr;
    std::vector<std::string> _portNames;

    int _arpMode = 0;
    std::set<unsigned int> notesDown;
    unsigned char _octaveShift = 3;
    unsigned char _velocity = 100;
    float _noteLength = 0.4f;
    float _bpm = 100;
    bool pauseMode = true;
    bool recordMode = true;
    std::vector<unsigned char> _notesToArp;
    size_t _currentNote = 0;

    void PianoKey(
        const char *label,
        int noteNumberInOctave,
        unsigned char velocity);

private:
    void *_windowHandle;
};

#endif // APP_H
