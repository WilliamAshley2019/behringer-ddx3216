#include <JuceHeader.h>
#include "MainComponent.h"

class DDX3216ControllerApplication : public juce::JUCEApplication
{
public:
    DDX3216ControllerApplication() = default;

    const juce::String getApplicationName() override    { return "DDX3216 Controller"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override           { return true; }

    void initialise (const juce::String&) override
    {
        mainWindow.reset (new MainWindow (getApplicationName()));
    }

    void shutdown() override { mainWindow = nullptr; }

    class MainWindow : public juce::DocumentWindow
    {
    public:
        explicit MainWindow (const juce::String& name)
            : DocumentWindow (name,
                               juce::Desktop::getInstance().getDefaultLookAndFeel()
                                   .findColour (juce::ResizableWindow::backgroundColourId),
                               DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new MainComponent(), true);
            setResizable (true, true);
            centreWithSize (getWidth(), getHeight());
            setVisible (true);
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION (DDX3216ControllerApplication)
