#include "PluginEditor.h"

namespace
{
    juce::Rectangle<int> canvasSizeFor (juce::Rectangle<int> available)
    {
        const int units = std::max (1, std::min (available.getWidth() / 4, available.getHeight() / 3));
        return { units * 4, units * 3 };
    }
}

PluginEditor::PluginEditor (PluginProcessor& owner)
    : AudioProcessorEditor (owner), processorRef (owner), nativeView (std::make_unique<phrases::ui::NativeEditor> (owner))
{
    addAndMakeVisible (*nativeView);
    nativeView->projectAction = [this] (int command) { projectCommand (command); };
    nativeView->fullscreenChanged = [this] { setEditorFullscreen (! editorFullscreen); };
   #if JUCE_IOS
    setSize (designWidth, designHeight);
   #else
    setResizable (true, true);
    const auto initialSize = canvasSizeFor (getDisplayUserBounds().reduced (24).withZeroOrigin()
                                               .getIntersection ({ designWidth, designHeight }));
    setSize (initialSize.getWidth(), initialSize.getHeight());
   #endif
    applyNormalResizeLimits();
    processorRef.getStateInformation (savedState);
}
PluginEditor::~PluginEditor() = default;

void PluginEditor::projectCommand (int command)
{
    const juce::Component::SafePointer<PluginEditor> safeThis (this);
    auto operation = [safeThis, command] {
        if (safeThis == nullptr)
            return;
        auto completion = [safeThis, command] (juce::var result) {
            if (safeThis == nullptr)
                return;
            if (static_cast<int> (result["success"]) == 0)
            {
                const auto error = result["error"].toString();
                if (error.isNotEmpty())
                    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "MIDI Phrases", error);
                return;
            }
            safeThis->processorRef.getStateInformation (safeThis->savedState);
            if (command == 2 || command == 5)
                safeThis->nativeView->synchronise();
            else
                safeThis->nativeView->documentChanged();
        };
        if (command == 0)
            safeThis->createNewProject (completion);
        else if (command == 1)
            safeThis->showLoadProjectDialog (completion);
        else if (command == 2)
            safeThis->saveProject (completion);
        else if (command == 5)
            safeThis->showSaveProjectDialog (completion);
        else
            safeThis->cycleProject (command == 3 ? -1 : 1, completion);
    };
    juce::MemoryBlock current;
    processorRef.getStateInformation (current);
    if (command != 2 && command != 5 && current != savedState)
    {
        juce::AlertWindow::showAsync (juce::MessageBoxOptions()
                                          .withIconType (juce::MessageBoxIconType::QuestionIcon)
                                          .withTitle ("Unsaved changes")
                                          .withMessage ("Discard changes to this project?")
                                          .withButton ("Discard")
                                          .withButton ("Cancel"),
            [operation] (int result) { if (result == 1) operation(); });
    }
    else
        operation();
}
void PluginEditor::paint (juce::Graphics& g) { g.fillAll (nativeView->palette.app); }
void PluginEditor::resized()
{
    auto available = getLocalBounds().toFloat();
   #if JUCE_IOS
    if (processorRef.hasStandaloneTransport())
    {
        if (const auto* display = juce::Desktop::getInstance().getDisplays().getDisplayForRect (getScreenBounds()))
        {
            const auto safeScreen = display->safeAreaInsets.subtractedFrom (display->userBounds.toNearestInt());
            const auto intersection = available.getIntersection (getLocalArea (nullptr, safeScreen).toFloat());
            if (! intersection.isEmpty())
                available = intersection;
        }
    }
   #endif
    if (available.isEmpty())
        return;

    const auto fit = fitDesignToAvailable (available.getWidth(), available.getHeight(), designWidth, designHeight);
    nativeView->setTransform (juce::AffineTransform::scale (fit.scale).translated (available.getX() + fit.x, available.getY() + fit.y));
    nativeView->setBounds (0, 0, designWidth, designHeight);
}

juce::File PluginEditor::getDefaultProjectsDirectory() const
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
        .getChildFile ("ofsound")
        .getChildFile ("MIDI Phrases");
}

juce::Array<juce::File> PluginEditor::getSiblingProjectFiles() const
{
    const auto directory = currentProjectFile.existsAsFile()
                               ? currentProjectFile.getParentDirectory()
                               : getDefaultProjectsDirectory();
    juce::Array<juce::File> files;

    if (directory.isDirectory())
        directory.findChildFiles (files, juce::File::findFiles, false, "*.midiphrases");

    std::sort (files.begin(), files.end(), [] (const juce::File& left, const juce::File& right) {
        return left.getFileName().compareNatural (right.getFileName(), true) < 0;
    });
    return files;
}

bool PluginEditor::hasPreviousProject() const
{
    return getSiblingProjectFiles().size() > 1;
}

bool PluginEditor::hasNextProject() const
{
    return getSiblingProjectFiles().size() > 1;
}

juce::String PluginEditor::getCurrentProjectFileName() const
{
    return currentProjectFile.existsAsFile() ? currentProjectFile.getFileName() : juce::String();
}

juce::var PluginEditor::projectOperationResult (const bool success,
    const juce::String& errorMessage) const
{
    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty ("success", success ? 1 : 0);
    object->setProperty ("error", errorMessage);

    if (success)
    {
        object->setProperty ("projectName", processorRef.getProjectName());
        object->setProperty ("projectDescription", processorRef.getProjectDescription());
        object->setProperty ("projectCreatedAt", processorRef.getProjectCreatedAt());
        object->setProperty ("projectModifiedAt", processorRef.getProjectModifiedAt());
        object->setProperty ("projectFileName", getCurrentProjectFileName());
        object->setProperty ("hasPreviousProject", hasPreviousProject() ? 1 : 0);
        object->setProperty ("hasNextProject", hasNextProject() ? 1 : 0);
    }

    return juce::var (object.release());
}

bool PluginEditor::loadProjectFile (const juce::File& file, juce::String& errorMessage)
{
    constexpr size_t maximumProjectBytes = 16 * 1024 * 1024;

    if (!file.existsAsFile())
    {
        errorMessage = "That project file no longer exists.";
        return false;
    }

    if (file.getSize() <= 0 || static_cast<juce::uint64> (file.getSize()) > maximumProjectBytes)
    {
        errorMessage = "The selected file is empty or too large to be a MIDI Phrases project.";
        return false;
    }

    juce::MemoryBlock data;
    if (!file.loadFileAsData (data))
    {
        errorMessage = "The project file could not be read.";
        return false;
    }

    const auto xml = juce::parseXML (
        juce::String::createStringFromData (data.getData(), static_cast<int> (data.getSize())));
    if (xml == nullptr || !xml->hasTagName ("MidiPhrases"))
    {
        errorMessage = "The selected file is not a MIDI Phrases project.";
        return false;
    }

    processorRef.setStateInformation (data.getData(), static_cast<int> (data.getSize()));
    currentProjectFile = file;

    if (!xml->hasAttribute ("projectName"))
    {
        processorRef.setProjectMetadata (file.getFileNameWithoutExtension(),
            {},
            {},
            {},
            "dark",
            100,
            processorRef.getProjectStretchStepsToFit());
    }
    else
    {
        const auto fileBaseName = file.getFileNameWithoutExtension();

        if (processorRef.getProjectName() == "Untitled Project"
            && fileBaseName.isNotEmpty()
            && fileBaseName != "Untitled Project")
        {
            processorRef.setProjectMetadata (fileBaseName,
                processorRef.getProjectDescription(),
                processorRef.getProjectCreatedAt(),
                processorRef.getProjectModifiedAt(),
                processorRef.getProjectThemeMode(),
                processorRef.getProjectUiScalePercent(),
                processorRef.getProjectStretchStepsToFit());
        }
    }

    return true;
}

bool PluginEditor::saveProjectFile (const juce::File& file, const juce::String& name, juce::String& errorMessage)
{
    if (!file.getParentDirectory().createDirectory())
    {
        errorMessage = "The project folder could not be created.";
        return false;
    }

    const auto oldName = processorRef.getProjectName();
    const auto oldCreated = processorRef.getProjectCreatedAt();
    const auto oldModified = processorRef.getProjectModifiedAt();
    const auto now = juce::Time::getCurrentTime().toISO8601 (true);
    const auto setMetadata = [this] (const juce::String& projectName, const juce::String& created, const juce::String& modified) {
        processorRef.setProjectMetadata (projectName, processorRef.getProjectDescription(), created, modified,
                                         processorRef.getProjectThemeMode(), processorRef.getProjectUiScalePercent(),
                                         processorRef.getProjectStretchStepsToFit());
    };
    setMetadata (name, oldCreated.isEmpty() ? now : oldCreated, now);
    juce::MemoryBlock data;
    processorRef.getStateInformation (data);
    juce::TemporaryFile temporaryFile (file);

    if (!temporaryFile.getFile().replaceWithData (data.getData(), data.getSize())
        || !temporaryFile.overwriteTargetFileWithTemporary())
    {
        setMetadata (oldName, oldCreated, oldModified);
        errorMessage = "The project file could not be written.";
        return false;
    }

    currentProjectFile = file;
    return true;
}

void PluginEditor::saveProject (Completion complete)
{
    if (projectFileChooser != nullptr)
    {
        complete (projectOperationResult (false, "A project dialog is already open."));
        return;
    }
    if (!currentProjectFile.existsAsFile())
    {
        showSaveProjectDialog (std::move (complete));
        return;
    }

    juce::String error;
    const bool saved = saveProjectFile (currentProjectFile, processorRef.getProjectName(), error);
    complete (projectOperationResult (saved, error));
}

void PluginEditor::showSaveProjectDialog (Completion complete)
{
    if (projectFileChooser != nullptr)
    {
        complete (projectOperationResult (false, "A project dialog is already open."));
        return;
    }

    const auto name = processorRef.getProjectName();
    const auto generation = processorRef.getDocumentGeneration();
    auto directory = currentProjectFile.existsAsFile() ? currentProjectFile.getParentDirectory()
                                                       : getDefaultProjectsDirectory();
    directory.createDirectory();
    auto legalName = juce::File::createLegalFileName (
        name.isNotEmpty() ? name : juce::String ("Untitled Project"));
    if (legalName.isEmpty())
        legalName = "Untitled Project";
    const auto suggestedFile = currentProjectFile.existsAsFile()
                                   ? currentProjectFile
                                   : directory.getChildFile (legalName + ".midiphrases");

    projectFileChooser = std::make_unique<juce::FileChooser> (
        "Save MIDI Phrases Project", suggestedFile, "*.midiphrases", true);
    const juce::Component::SafePointer<PluginEditor> safeThis (this);
    projectFileChooser->launchAsync (
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
            | juce::FileBrowserComponent::warnAboutOverwriting,
        [safeThis, completion = std::move (complete), generation] (const juce::FileChooser& chooser) mutable {
            if (safeThis == nullptr)
                return;

            auto file = chooser.getResult();
            safeThis->projectFileChooser.reset();
            if (file == juce::File())
            {
                completion (safeThis->projectOperationResult (false));
                return;
            }
            if (generation != safeThis->processorRef.getDocumentGeneration())
            {
                completion (safeThis->projectOperationResult (false, "The project changed while the save dialog was open. Please save again."));
                return;
            }

            if (!file.hasFileExtension (".midiphrases"))
                file = file.withFileExtension (".midiphrases");

            const auto previousProjectFile = safeThis->currentProjectFile;
            const auto savedToNewPath = !previousProjectFile.existsAsFile()
                                        || previousProjectFile != file;
            const auto projectNameToSave = savedToNewPath ? file.getFileNameWithoutExtension()
                                                         : safeThis->processorRef.getProjectName();
            juce::String error;
            const auto saved = safeThis->saveProjectFile (file, projectNameToSave, error);
            completion (safeThis->projectOperationResult (saved, error));
        });
}

void PluginEditor::showLoadProjectDialog (
    Completion complete)
{
    if (projectFileChooser != nullptr)
    {
        complete (projectOperationResult (false, "A project dialog is already open."));
        return;
    }

    const auto start = currentProjectFile.existsAsFile() ? currentProjectFile
                                                         : getDefaultProjectsDirectory();
    projectFileChooser = std::make_unique<juce::FileChooser> (
        "Load MIDI Phrases Project", start, "*.midiphrases", true);
    const juce::Component::SafePointer<PluginEditor> safeThis (this);
    projectFileChooser->launchAsync (
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safeThis, completion = std::move (complete)] (const juce::FileChooser& chooser) mutable {
            if (safeThis == nullptr)
                return;

            const auto file = chooser.getResult();
            safeThis->projectFileChooser.reset();
            if (file == juce::File())
            {
                completion (safeThis->projectOperationResult (false));
                return;
            }

            juce::String error;
            const auto loaded = safeThis->loadProjectFile (file, error);
            completion (safeThis->projectOperationResult (loaded, error));
        });
}

void PluginEditor::createNewProject (
    Completion complete)
{
    if (projectFileChooser != nullptr)
    {
        complete (projectOperationResult (false, "Close the open project dialog first."));
        return;
    }

    processorRef.resetProject();
    currentProjectFile = juce::File();
    complete (projectOperationResult (true));
}

void PluginEditor::cycleProject (
    const int direction,
    Completion complete)
{
    const auto files = getSiblingProjectFiles();

    if (files.size() <= 1)
    {
        complete (projectOperationResult (
            false,
            files.isEmpty() ? juce::String ("No saved projects were found in the projects folder.")
                            : juce::String()));
        return;
    }

    auto currentIndex = files.indexOf (currentProjectFile);
    int nextIndex = 0;

    if (currentIndex < 0)
    {
        nextIndex = direction < 0 ? files.size() - 1 : 0;
    }
    else
    {
        nextIndex = currentIndex + (direction < 0 ? -1 : 1);

        if (nextIndex < 0)
            nextIndex = files.size() - 1;
        else if (nextIndex >= files.size())
            nextIndex = 0;
    }

    if (currentIndex >= 0 && nextIndex == currentIndex)
    {
        complete (projectOperationResult (false));
        return;
    }

    juce::String error;
    const auto loaded = loadProjectFile (files[nextIndex], error);
    complete (projectOperationResult (loaded, error));
}

void PluginEditor::applyNormalResizeLimits()
{
   #if JUCE_IOS
    // The OS and AUv3 host provide their own viewport.
    setResizeLimits (1, 1, 100000, 100000);
   #else
    setResizeLimits (400, 300, 4096, 3072);
    getConstrainer()->setFixedAspectRatio (static_cast<double> (designWidth) / designHeight);
   #endif
}

void PluginEditor::applyFullscreenResizeLimits (const juce::Rectangle<int> targetBounds)
{
    if (auto* limits = getConstrainer())
        limits->setFixedAspectRatio (0.0);
    setResizeLimits (1, 1, juce::jmax (4096, targetBounds.getWidth()), juce::jmax (3072, targetBounds.getHeight()));
}

juce::Rectangle<int> PluginEditor::getDisplayUserBounds() const
{
    const auto& displays = juce::Desktop::getInstance().getDisplays();

    if (const auto* display = displays.getDisplayForRect (getScreenBounds()))
        return display->userBounds.toNearestInt();

    return { 0, 0, juce::jmax (getWidth(), 2000), juce::jmax (getHeight(), 1280) };
}

juce::ResizableWindow* PluginEditor::getStandaloneFullscreenWindow() const
{
    if (!processorRef.hasStandaloneTransport())
        return nullptr;

    auto* topLevelComponent = getTopLevelComponent();

    if (topLevelComponent == nullptr || topLevelComponent == this)
        return nullptr;

    return dynamic_cast<juce::ResizableWindow*> (topLevelComponent);
}

juce::DocumentWindow* PluginEditor::getStandaloneDocumentWindow() const
{
    if (!processorRef.hasStandaloneTransport())
        return nullptr;

    auto* topLevelComponent = getTopLevelComponent();

    if (topLevelComponent == nullptr || topLevelComponent == this)
        return nullptr;

    return dynamic_cast<juce::DocumentWindow*> (topLevelComponent);
}

void PluginEditor::setStandaloneWrapperChromeVisible (const bool shouldBeVisible)
{
    auto* standaloneWindow = getStandaloneDocumentWindow();

    if (standaloneWindow == nullptr)
        return;

    if (!shouldBeVisible)
    {
        if (standaloneTitleBarHeightBeforeFullscreen < 0)
            standaloneTitleBarHeightBeforeFullscreen = standaloneWindow->getTitleBarHeight();

        standaloneWindow->setTitleBarButtonsRequired (0, false);
        standaloneWindow->setTitleBarHeight (0);
    }
    else
    {
        standaloneWindow->setTitleBarHeight (standaloneTitleBarHeightBeforeFullscreen > 0
                                                 ? standaloneTitleBarHeightBeforeFullscreen
                                                 : 26);
        standaloneWindow->setTitleBarButtonsRequired (juce::DocumentWindow::minimiseButton | juce::DocumentWindow::closeButton,
            false);
        standaloneTitleBarHeightBeforeFullscreen = -1;
    }

    for (int childIndex = 0; childIndex < standaloneWindow->getNumChildComponents(); ++childIndex)
    {
        if (auto* button = dynamic_cast<juce::Button*> (standaloneWindow->getChildComponent (childIndex)))
        {
            if (button->getButtonText() == "Options")
                button->setVisible (shouldBeVisible);
        }
    }

    standaloneWindow->resized();
}

juce::var PluginEditor::createEditorFullscreenState() const
{
    const auto* standaloneWindow = getStandaloneFullscreenWindow();
    const auto nativeFullscreenActive = standaloneWindow != nullptr && standaloneWindow->isFullScreen();

    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty ("enabled", editorFullscreen || nativeFullscreenActive ? 1 : 0);
    object->setProperty ("native", nativeFullscreenActive ? 1 : 0);
    object->setProperty ("available", 1);
    object->setProperty ("width", getWidth());
    object->setProperty ("height", getHeight());
    return juce::var (object.release());
}

juce::var PluginEditor::setEditorFullscreen (const bool shouldBeFullscreen)
{
    if (shouldBeFullscreen == editorFullscreen)
        return createEditorFullscreenState();

    if (shouldBeFullscreen)
    {
        preFullscreenEditorBounds = getBounds();
        editorFullscreen = true;
        const auto displayBounds = getDisplayUserBounds();
        applyFullscreenResizeLimits (displayBounds);

        if (auto* standaloneWindow = getStandaloneFullscreenWindow())
        {
            standaloneNativeFullscreen = true;
            setStandaloneWrapperChromeVisible (false);
            standaloneWindow->setFullScreen (true);
            standaloneWindow->setContentComponentSize (displayBounds.getWidth(), displayBounds.getHeight());
            resized();
            return createEditorFullscreenState();
        }

        const auto size = canvasSizeFor (displayBounds);
        setSize (size.getWidth(), size.getHeight());
        resized();
        return createEditorFullscreenState();
    }

    if (standaloneNativeFullscreen)
    {
        if (auto* standaloneWindow = getStandaloneFullscreenWindow())
            standaloneWindow->setFullScreen (false);

        standaloneNativeFullscreen = false;
        setStandaloneWrapperChromeVisible (true);
    }

    const auto restoreBounds = preFullscreenEditorBounds;
    editorFullscreen = false;
    applyNormalResizeLimits();

    if (!restoreBounds.isEmpty())
        setSize (restoreBounds.getWidth(), restoreBounds.getHeight());

    resized();
    return createEditorFullscreenState();
}

juce::var PluginEditor::handleEditorFullscreenRequest (const int mode)
{
    if (mode < 0)
        return createEditorFullscreenState();

    return setEditorFullscreen (mode != 0);
}
