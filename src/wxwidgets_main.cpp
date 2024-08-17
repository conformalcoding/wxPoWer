#include <cstdio>
#include <cinttypes>

#include <sstream>
#include <stack>
#include <thread>

#include "wx/wx.h"
#include "wx/cmdline.h"
#include "wx/notebook.h"
#include "wx/spinctrl.h"

#include "power.hpp"

namespace wxpower
{
    ////////////////////////////////////////////////////////////////////////////
    // wxPowerFrame
    ////////////////////////////////////////////////////////////////////////////

    class wxPowerFrame : public wxFrame
    {
    public:
        wxPowerFrame(const wxString& title);

        // TODO
        /*void ReadConfig(const wxPowerConfig& config);
        void WriteConfig(wxPowerConfig& config) const;*/

        void OnAbout(wxCommandEvent& event);
        void OnButton(wxCommandEvent& event);
        void OnCheckBox(wxCommandEvent& event);
        void OnQuit(wxCommandEvent& event);
        void OnTimer(wxTimerEvent& event);

        void BeginProveV0();
        void DumpProveV0Info(
            std::stringstream& ss, const ProveV0Manager* state,
            const ProveV0Manager::MasterGuarded& masterGuarded,
            bool printBestProof = false);
        void EnableProveElements();
        void DisableProveElements();

    private:
        DECLARE_EVENT_TABLE()

        ProveV0Manager* proveV0Mgr = nullptr;
        ProveV0Manager::State proveV0LastState = ProveV0Manager::State::finished;

        wxTimer updateTimer;

        wxMenu* fileMenu = nullptr;
        wxMenu* helpMenu = nullptr;
        wxMenuBar* menuBar = nullptr;
        wxNotebook* topNotebook = nullptr;
        wxPanel* proveV0Panel = nullptr;
        wxPanel* verifyPanel = nullptr;
        wxPanel* settingsPanel = nullptr;

        // Prove

        wxBoxSizer* proveV0Sizer = nullptr;
        wxBoxSizer* proveV0SizerTop = nullptr;
        wxGridSizer* proveV0SizerMid = nullptr;
        wxBoxSizer* proveV0SizerBot = nullptr;

        wxTextCtrl* proveV0Msg = nullptr;
        wxStaticText* proveV0UserIdLabel = nullptr;
        wxTextCtrl* proveV0UserId = nullptr;
        wxStaticText* proveV0ContextLabel = nullptr;
        wxTextCtrl* proveV0Context = nullptr;
        wxCheckBox* proveV0UseDiff = nullptr;
        wxSpinCtrl* proveV0Diff = nullptr;
        wxCheckBox* proveV0UseTimeLimit = nullptr;
        wxSpinCtrl* proveV0TimeLimit = nullptr;
        wxButton* proveV0Prove = nullptr;

        wxTextCtrl* proveV0Output = nullptr;

        // Verify

        wxBoxSizer* verifySizer = nullptr;
        wxBoxSizer* verifySizerLeft = nullptr;

        wxTextCtrl* verifyInput = nullptr;
        wxButton* verifySubmit = nullptr;
        wxTextCtrl* verifyOutput = nullptr;

        // Settings

        wxBoxSizer* settingsSizer = nullptr;
        wxBoxSizer* settingsSizerOther = nullptr;

        wxString* settingsCoreChoiceList = nullptr;
        wxCheckListBox* settingsInitCores = nullptr;
        wxCheckListBox* settingsHashCores = nullptr;
        wxCheckBox* settingsLargePages = nullptr;
        wxButton* settingsBenchmark = nullptr;
    };

    BEGIN_EVENT_TABLE(wxPowerFrame, wxFrame)
        EVT_BUTTON(wxID_ANY, wxPowerFrame::OnButton)
        EVT_CHECKBOX(wxID_ANY, wxPowerFrame::OnCheckBox)
        EVT_MENU(wxID_ABOUT, wxPowerFrame::OnAbout)
        EVT_MENU(wxID_EXIT, wxPowerFrame::OnQuit)
        EVT_TIMER(0, wxPowerFrame::OnTimer)
    END_EVENT_TABLE()

    wxPowerFrame::wxPowerFrame(const wxString& title) :
        wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(600, 400)),
        updateTimer(this, 0)
    {
        fileMenu = new wxMenu;
        helpMenu = new wxMenu;

        fileMenu->Append(wxID_EXIT, wxT("E&xit"));
        helpMenu->Append(wxID_ABOUT, wxT("&About..."));

        menuBar = new wxMenuBar;

        menuBar->Append(fileMenu, wxT("&File"));
        menuBar->Append(helpMenu, wxT("&Help"));

        SetMinSize(wxSize(700, 500));

        SetMenuBar(menuBar);

        CreateStatusBar(2);
        SetStatusText(wxT("Welcome to wxPoWer!"));

        topNotebook = new wxNotebook(this, wxID_ANY);

        ////////////////////////////////////////////////////////////////////////
        // Prove v0
        ////////////////////////////////////////////////////////////////////////

        proveV0Panel = new wxPanel(topNotebook, wxID_ANY);
        topNotebook->AddPage(proveV0Panel, wxT("Prove (v0)"), true);

        proveV0Sizer = new wxBoxSizer(wxHORIZONTAL);
        proveV0SizerTop = new wxBoxSizer(wxVERTICAL);
        proveV0SizerMid = new wxGridSizer(2);
        proveV0SizerBot = new wxBoxSizer(wxVERTICAL);

        proveV0Msg = new wxTextCtrl(
            proveV0Panel, wxID_ANY, wxEmptyString, wxDefaultPosition,
            wxDefaultSize, wxTE_MULTILINE);
        proveV0Msg->SetToolTip(wxT("Message to prove"));

        proveV0UserIdLabel = new wxStaticText(
            proveV0Panel, wxID_ANY, wxT("User ID"), wxDefaultPosition,
            wxDefaultSize, wxST_ELLIPSIZE_END);

        proveV0UserId = new wxTextCtrl(proveV0Panel, wxID_ANY);
        proveV0UserId->SetToolTip(wxT("User ID"));

        proveV0ContextLabel = new wxStaticText(
            proveV0Panel, wxID_ANY, wxT("Context"), wxDefaultPosition,
            wxDefaultSize, wxST_ELLIPSIZE_END);

        proveV0Context = new wxTextCtrl(proveV0Panel, wxID_ANY);
        proveV0Context->SetToolTip(wxT("Context"));

        proveV0UseDiff = new wxCheckBox(
            proveV0Panel, wxID_ANY, wxT("Difficulty limit"));
        proveV0UseDiff->SetValue(true);

        proveV0Diff = new wxSpinCtrl(
            proveV0Panel, wxID_ANY, wxT("15"), wxDefaultPosition,
            wxDefaultSize, wxSP_ARROW_KEYS | wxALIGN_RIGHT);
        proveV0Diff->SetRange(0, 256);
        proveV0Diff->SetToolTip(wxT("Difficulty, range [0, 256]"));

        proveV0UseTimeLimit = new wxCheckBox(
            proveV0Panel, wxID_ANY, wxT("Time limit"));
        proveV0UseTimeLimit->SetValue(true);

        proveV0TimeLimit = new wxSpinCtrl(
            proveV0Panel, wxID_ANY, wxT("120"), wxDefaultPosition,
            wxDefaultSize, wxSP_ARROW_KEYS | wxALIGN_RIGHT);
        proveV0TimeLimit->SetRange(1, 86400);
        proveV0TimeLimit->SetToolTip(wxT("Time limit (seconds), range [1, 86400]"));

        proveV0Prove = new wxButton(proveV0Panel, wxID_ANY, wxT("Prove"));

        proveV0Output = new wxTextCtrl(
            proveV0Panel, wxID_ANY, wxEmptyString, wxDefaultPosition,
            wxDefaultSize, wxTE_MULTILINE);
        proveV0Output->SetEditable(false);
        proveV0Output->SetToolTip(wxT("Proof output"));

        proveV0SizerMid->Add(
            proveV0UserIdLabel, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
        proveV0SizerMid->Add(proveV0UserId, 2, wxEXPAND);
        proveV0SizerMid->Add(
            proveV0ContextLabel, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
        proveV0SizerMid->Add(proveV0Context, 2, wxEXPAND);
        proveV0SizerMid->Add(
            proveV0UseDiff, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
        proveV0SizerMid->Add(proveV0Diff, 2, wxEXPAND);
        proveV0SizerMid->Add(
            proveV0UseTimeLimit, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
        proveV0SizerMid->Add(proveV0TimeLimit, 2, wxEXPAND);

        proveV0SizerTop->Add(proveV0Msg, 1, wxEXPAND);
        proveV0SizerTop->Add(proveV0SizerMid, 0, wxALIGN_LEFT);
        proveV0SizerTop->Add(proveV0Prove, 0, wxEXPAND);

        proveV0SizerBot->Add(proveV0Output, 1, wxEXPAND);

        proveV0Sizer->Add(proveV0SizerTop, 2, wxEXPAND);
        proveV0Sizer->Add(proveV0SizerBot, 2, wxEXPAND);

        proveV0Panel->SetSizerAndFit(proveV0Sizer);

        ////////////////////////////////////////////////////////////////////////
        // Verify
        ////////////////////////////////////////////////////////////////////////

        verifyPanel = new wxPanel(topNotebook, wxID_ANY);
        topNotebook->AddPage(verifyPanel, wxT("Verify"), false);

        verifySizer = new wxBoxSizer(wxHORIZONTAL);
        verifySizerLeft = new wxBoxSizer(wxVERTICAL);

        verifyInput = new wxTextCtrl(
            verifyPanel, wxID_ANY, wxEmptyString, wxDefaultPosition,
            wxDefaultSize, wxTE_MULTILINE);

        verifySubmit = new wxButton(verifyPanel, wxID_ANY, wxT("Verify"));

        verifyOutput = new wxTextCtrl(
            verifyPanel, wxID_ANY, wxEmptyString, wxDefaultPosition,
            wxDefaultSize, wxTE_MULTILINE);
        verifyOutput->SetEditable(false);

        verifySizerLeft->Add(verifyInput, 1, wxEXPAND);
        verifySizerLeft->Add(verifySubmit, 0, wxEXPAND);

        verifySizer->Add(verifySizerLeft, 1, wxEXPAND);
        verifySizer->Add(verifyOutput, 1, wxEXPAND);
        verifyPanel->SetSizerAndFit(verifySizer);

        ////////////////////////////////////////////////////////////////////////
        // Settings
        ////////////////////////////////////////////////////////////////////////

        settingsPanel = new wxPanel(topNotebook, wxID_ANY);
        topNotebook->AddPage(settingsPanel, wxT("Settings"), false);

        settingsSizer = new wxBoxSizer(wxHORIZONTAL);
        settingsSizerOther = new wxBoxSizer(wxVERTICAL);

        const unsigned int maxProcs = std::thread::hardware_concurrency();
        settingsCoreChoiceList = new wxString[maxProcs];

        {
            char temp[32];

            for (unsigned int i = 0; i < maxProcs; i++)
            {
                snprintf(temp, 31, "CPU %u", i);
                temp[31] = 0;
                settingsCoreChoiceList[i] = temp;
            }
        }

        settingsInitCores = new wxCheckListBox(
            settingsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
            maxProcs, settingsCoreChoiceList);
        settingsInitCores->SetToolTip("Init cores");

        settingsHashCores = new wxCheckListBox(
            settingsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
            maxProcs, settingsCoreChoiceList);
        settingsHashCores->SetToolTip("Hash cores");

        // TODO: replace with saved state
        for (unsigned int i = 2; i < maxProcs; i += 2)
        {
            settingsInitCores->Check(i);
            settingsHashCores->Check(i);
        }

        settingsLargePages = new wxCheckBox(
            settingsPanel, wxID_ANY, wxT("Use large pages"));
        settingsLargePages->SetValue(true);

        settingsSizerOther->Add(settingsLargePages);

        settingsSizer->Add(settingsInitCores, 0, wxEXPAND);
        settingsSizer->Add(settingsHashCores, 0, wxEXPAND);
        settingsSizer->Add(settingsSizerOther, 1, wxEXPAND);

        settingsPanel->SetSizerAndFit(settingsSizer);

        updateTimer.Start(500);
    }

    void wxPowerFrame::OnAbout(wxCommandEvent&)
    {
        wxString msg;
        msg.Printf(wxT("Hello and welcome to %s"), wxVERSION_STRING);

        wxMessageBox(
            msg, wxT("About wxPoW"), wxOK | wxICON_INFORMATION, this);
    }

    void wxPowerFrame::OnButton(wxCommandEvent& event)
    {
        if (event.GetEventObject() == proveV0Prove)
        {
            if ((proveV0LastState == ProveV0Manager::State::rxFailed) ||
                (proveV0LastState == ProveV0Manager::State::rxCancelled) ||
                (proveV0LastState == ProveV0Manager::State::hashCancelled) ||
                (proveV0LastState == ProveV0Manager::State::finished))
            {
                assert(!proveV0Mgr);

                BeginProveV0();
                DisableProveElements();
            }
            else
            {
                assert((proveV0LastState == ProveV0Manager::State::rxIniting) ||
                    (proveV0LastState == ProveV0Manager::State::hashing));
                assert(proveV0Mgr);
                
                proveV0Prove->SetLabel(wxT("Cancelling..."));
                proveV0Prove->Disable();
                proveV0Mgr->cancel();
            }
        }
        else if (event.GetEventObject() == verifySubmit)
        {
            const std::string proof = verifyInput->GetValue().utf8_string();
            std::optional<HashResult> res;
            std::string error;
            std::stack<u32> versions;
            bool foundVersion = false;
            std::stringstream ss;

            ss << "\n----------------------------------------";
            ss << "\nNew verification.\n\nTrying proof spec versions:";

            for (u32 i = 0; i <= PowerBase::latestVersion; i++)
            {
                versions.push(i);

                if (i > 0)
                    ss << ",";

                ss << " " << i;
            }

            ss << "...\n";

            while ((versions.size() > 0) && (!foundVersion))
            {
                const u32 thisVersion = versions.top();
                versions.pop();

                PowerBase* thisPower = PowerBase::createInstance(thisVersion);

                if (thisPower)
                {
                    std::string prettyMetaData;
                    const auto tic = NOW;

                    if (thisPower->verifyMessage(
                        proof, settingsLargePages->GetValue(), res, prettyMetaData, error))
                    {
                        const auto toc = NOW;
                        foundVersion = true;
                        ss << "\nThis proof uses version " << thisVersion
                            << ".";

                        if (res.has_value())
                        {
                            ss << "\n\n" << prettyMetaData
                                << "\n\nHash: " << res->hash.toString()
                                << " (diff: " << res->diff << ")";
                        }
                        else
                        {
                            ss << "\nError while verifying: " << error;
                        }

                        ss << "\n\nVerifying took " << SECS(toc - tic) << " seconds.";
                    }

                    delete thisPower;
                }
            }

            if (!foundVersion)
                ss << "\nUnable to identify proof spec version.";

            verifyOutput->AppendText(wxString::FromUTF8(ss.str()));
        }
        else
        {
            wxMessageBox(
                wxT("Unknown button pressed"), wxT("Error"),
                wxOK | wxICON_ERROR, this);
        }
    }

    void wxPowerFrame::BeginProveV0()
    {
        std::stringstream ss;

        std::string body = proveV0Msg->GetValue().utf8_string();
        PowerV0::trimBody(body);
        proveV0Msg->SetValue(wxString::FromUTF8(body));

        std::string userId = proveV0UserId->GetValue().utf8_string();
        PowerV0::trimBody(userId);
        proveV0UserId->SetValue(wxString::FromUTF8(userId));

        std::string context = proveV0Context->GetValue().utf8_string();
        PowerV0::trimBody(context);
        proveV0Context->SetValue(wxString::FromUTF8(context));

        PowerV0::ProofContent content {
            body, 0, userId, context
        };

        ss << "\n\n----------------------------------------"
            << "\nGenerating new proof:"
            << "\n\n---- BEGIN BODY ----\n"
            << content.body
            << "\n----END BODY----\n"
            << "\nUser ID: " << content.userId
            << "\nContext: " << content.context;

        std::vector<u32> initCores;
        std::vector<u32> hashCores;

        // Scope
        {
            wxArrayInt wxInitCores;
            settingsInitCores->GetCheckedItems(wxInitCores);

            ss << "\nInit cores:";

            for (u32 i = 0; i < wxInitCores.GetCount(); i++)
            {
                initCores.push_back(wxInitCores.Item(i));
                ss << " " << initCores.back();
            }
        }

        // Scope
        {
            wxArrayInt wxHashCores;
            settingsHashCores->GetCheckedItems(wxHashCores);

            ss << "\nHash cores:";

            for (u32 i = 0; i < wxHashCores.GetCount(); i++)
            {
                hashCores.push_back(wxHashCores.Item(i));
                ss << " " << hashCores.back();
            }
        }

        ss << "\n";

        std::optional<u32> diff;

        if (proveV0UseDiff->GetValue())
            diff.emplace(static_cast<u32>(proveV0Diff->GetValue()));

        std::optional<double> timeLimit;

        if (proveV0UseTimeLimit->GetValue())
            timeLimit.emplace(static_cast<double>(proveV0TimeLimit->GetValue()));

        proveV0Output->AppendText(wxString::FromUTF8(ss.str()));

        proveV0Mgr = new ProveV0Manager(
            content, initCores, hashCores,
            settingsLargePages->GetValue(), diff, timeLimit);
    }

    void wxPowerFrame::EnableProveElements()
    {
        proveV0Msg->Enable();
        proveV0UserId->Enable();
        proveV0Context->Enable();
        proveV0UseDiff->Enable();
        proveV0Diff->Enable();
        proveV0UseTimeLimit->Enable();
        proveV0TimeLimit->Enable();
        proveV0Prove->Enable();
        proveV0Prove->SetLabel(wxT("Prove"));

        settingsInitCores->Enable();
        settingsHashCores->Enable();
        settingsLargePages->Enable();
    }

    void wxPowerFrame::DisableProveElements()
    {
        proveV0Msg->Disable();
        proveV0UserId->Disable();
        proveV0Context->Disable();
        proveV0UseDiff->Disable();
        proveV0Diff->Disable();
        proveV0UseTimeLimit->Disable();
        proveV0TimeLimit->Disable();
        proveV0Prove->Disable();

        settingsInitCores->Disable();
        settingsHashCores->Disable();
        settingsLargePages->Disable();
    }

    void wxPowerFrame::OnCheckBox(wxCommandEvent& event)
    {
        if (event.GetEventObject() == proveV0UseDiff)
            proveV0Diff->Show(proveV0UseDiff->GetValue());
        else if (event.GetEventObject() == proveV0UseTimeLimit)
            proveV0TimeLimit->Show(proveV0UseTimeLimit->GetValue());
    }

    void wxPowerFrame::OnQuit(wxCommandEvent&)
    {
        updateTimer.Stop();
        Close();
    }

    void wxPowerFrame::OnTimer(wxTimerEvent&)
    {
        using State = ProveV0Manager::State;

        if (proveV0Mgr)
        {
            const ProveV0Manager::MasterGuarded masterGuarded =
                proveV0Mgr->getMasterGuardedData();
            const State newState = proveV0Mgr->getState();

            if (newState == State::rxFailed)
            {
                // Failure or cancellation on startup?

                std::stringstream ss;

                ss << "\nA problem occurred when proving. See errors/warnings below.";

                if (!masterGuarded.errorStr.empty())
                    ss << "\nError: \"" << masterGuarded.errorStr << "\"";

                for (size_t i = 0; i < masterGuarded.warnings.size(); i++)
                    ss << "\nWarning: \"" << masterGuarded.warnings.at(i) << "\"";

                proveV0Output->AppendText(wxString::FromUTF8(ss.str()));
                EnableProveElements();

                delete proveV0Mgr;
                proveV0Mgr = nullptr;
            }
            else if (newState == State::rxIniting)
            {
                // Initializing

                proveV0Output->AppendText(wxT("\nInitializing..."));
            }
            else if (newState == State::hashing)
            {
                // Hashing

                assert(masterGuarded.hashStartTime.has_value());
                std::stringstream ss;

                if (proveV0LastState == State::rxIniting)
                {
                    ss << "\nInitialization finished - time: "
                        << masterGuarded.rxTime.value()
                        << " s";
                }

                ss << "\nHashing progress:";
                DumpProveV0Info(ss, proveV0Mgr, masterGuarded);

                ss << "\nHashing time so far: "
                    << SECS(NOW - masterGuarded.hashStartTime.value())
                    << " seconds.";

                proveV0Output->AppendText(wxString::FromUTF8(ss.str()));
            }
            else if ((newState == State::hashCancelled) ||
                (newState == State::finished))
            {
                assert(masterGuarded.hashStopTime.has_value());

                std::stringstream ss;

                ss << "\n\n----------------------------------------";

                if (newState == State::hashCancelled)
                    ss << "\nHashing was cancelled early.";

                ss << "\nHashing info:";
                DumpProveV0Info(ss, proveV0Mgr, masterGuarded, true);

                ss << "\nTotal hashing time: "
                    << SECS(masterGuarded.hashStopTime.value()
                        - masterGuarded.hashStartTime.value())
                    << " seconds.";

                proveV0Output->AppendText(wxString::FromUTF8(ss.str()));
                EnableProveElements();

                delete proveV0Mgr;
                proveV0Mgr = nullptr;
            }
            else if (newState == State::rxCancelled)
            {
                assert(masterGuarded.rxTime.has_value());

                std::stringstream ss;

                ss << "\nInitialization was cancelled - total time: "
                    << masterGuarded.rxTime.value() << " s";

                proveV0Output->AppendText(wxString::FromUTF8(ss.str()));
                EnableProveElements();

                delete proveV0Mgr;
                proveV0Mgr = nullptr;
            }
            else
            {
                assert(false && "Unknown state");
            }

            /* If this is the first timer fire since starting a new prove and
               we didn't immediately hit a stopped state, make the button
               available for cancellation. */
            if (ProveV0Manager::isStoppedState(proveV0LastState) &&
                !ProveV0Manager::isStoppedState(newState))
            {
                proveV0Prove->SetLabel(wxT("Cancel"));
                proveV0Prove->Enable();
            }

            proveV0LastState = newState;
        }
        else
        {
            assert(
                (proveV0LastState == State::rxFailed) ||
                (proveV0LastState == State::rxCancelled) ||
                (proveV0LastState == State::hashCancelled) ||
                (proveV0LastState == State::finished));
        }
    }

    void wxPowerFrame::DumpProveV0Info(
        std::stringstream& ss, const ProveV0Manager* mgr,
        const ProveV0Manager::MasterGuarded& masterGuarded, bool printBestProof)
    {
        char temp[200];
        u32 bestThread = 0;
        u64 totalHashes = 0;
        u32 bestDiff = 0;
        const ProveV0Manager::ThreadGuardedRet threadGuarded = mgr->getThreadGuardedData();

        for (u32 t = 0; t < mgr->hashThreadCount; t++)
        {
            const u32 thisDiff = threadGuarded.bestDiff.at(t);

            snprintf(temp, 199,
                "\n>>> Thread %3" PRIu32 ": %8" PRIu64 " hashes, best diff %2" PRIu32,
                t, threadGuarded.hashes.at(t), thisDiff);

            temp[199] = 0;
            ss << temp;

            totalHashes += threadGuarded.hashes.at(t);

            if (thisDiff > bestDiff)
            {
                bestThread = t;
                bestDiff = thisDiff;
            }
        }

        snprintf(temp, 199,
            "\n\nTotal hashes: %8" PRIu64 ", best diff: %3" PRIu32,
            totalHashes, bestDiff);

        if (masterGuarded.hashStartTime.has_value())
        {
            double currHashTime = -1.0;

            if (masterGuarded.hashStopTime.has_value())
            {
                currHashTime = SECS(
                    masterGuarded.hashStopTime.value() -
                    masterGuarded.hashStartTime.value());
            }
            else
            {
                currHashTime = SECS(
                    NOW - masterGuarded.hashStartTime.value());
            }

            assert(currHashTime >= 0.0);

            if (currHashTime > 0.0)
            {
                const double hps = static_cast<double>(totalHashes) / currHashTime;
                const double hpspc = hps / static_cast<double>(mgr->hashThreadCount);

                ss << "\nHash rate (all cores): " << hps
                    << "\nHash rate (avg per core): " << hpspc;
            }
        }

        if (printBestProof)
        {
            const HashResult& bestResult = masterGuarded.bestResults.at(bestThread);

            ss << "\n\nBest result:";
            ss << "\n\n---- BEGIN PROOF ----\n";
            ss << bestResult.proof;
            ss << "\n----END PROOF----\n";
            ss << "\nHash: " << bestResult.hash.toString();
            ss << " (diff: " << bestResult.diff << ")";
        }

        temp[199] = 0;
        ss << temp;
    }

    ////////////////////////////////////////////////////////////////////////////
    // wxPowerApp
    ////////////////////////////////////////////////////////////////////////////

    class wxPowerApp : public wxApp
    {
    public:
        bool OnInit() override;
        void OnInitCmdLine(wxCmdLineParser& parser) override;
        bool OnCmdLineParsed(wxCmdLineParser& parser) override;

    private:
        wxPowerFrame* frame = nullptr;
    };

    bool wxPowerApp::OnInit()
    {
        frame = new wxPowerFrame(wxT("wxPoWer"));
        frame->Show();
        return true;
    }

    void wxPowerApp::OnInitCmdLine(wxCmdLineParser& parser)
    {
        static const wxCmdLineEntryDesc cmdLine[] = {
            { wxCMD_LINE_PARAM, nullptr, nullptr, "config file", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
            { wxCMD_LINE_NONE }
        };

        parser.SetDesc(cmdLine);
        parser.SetSwitchChars(wxT("-"));
    }

    bool wxPowerApp::OnCmdLineParsed(wxCmdLineParser& parser)
    {
        bool ret = true;

        if (parser.GetParamCount() == 1)
        {

        }
        else if (parser.GetParamCount() != 0)
        {
            wxMessageBox(wxT("Bad command-line parameters"), wxT("Error"), wxOK | wxICON_ERROR);
            ret = false;
        }

        return ret;
    }
}

DECLARE_APP(wxpower::wxPowerApp)
IMPLEMENT_APP(wxpower::wxPowerApp)
