#include <opencv2/opencv.hpp>
#include "CollageSpecDialog.h"

namespace Wxiv
{
    /**
     * @brief Mostly by ChatGPT (all except fontFaceMap).
    */
    CollageSpecDialog::CollageSpecDialog(wxWindow* parent, ImageUtil::CollageSpec& spec)
        : wxDialog(parent, wxID_ANY, _("Collage Spec"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
        m_spec(spec)
    {
        wxFlexGridSizer* gridSizer = new wxFlexGridSizer(2, 5, 5);
        gridSizer->AddGrowableCol(1);

        // Row of controls for colCount
        wxStaticText* colCountLabel = new wxStaticText(this, wxID_ANY, _("Columns:"));
        m_colCountCtrl = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, m_spec.colCount);

        // Row of controls for imageWidthPx
        wxStaticText* imageWidthPxLabel = new wxStaticText(this, wxID_ANY, _("Image Width (px):"));
        m_imageWidthPxCtrl = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 100, 8192, m_spec.imageWidthPx);

        // Row of controls for marginPx
        wxStaticText* marginPxLabel = new wxStaticText(this, wxID_ANY, _("Margin (px):"));
        m_marginPxCtrl = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 256, m_spec.marginPx);

        // Row of controls for fontScale
        wxStaticText* fontScaleLabel = new wxStaticText(this, wxID_ANY, _("Font Scale:"));
        m_fontScaleCtrl = new wxSpinCtrlDouble(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0.1, 10.0, m_spec.fontScale, 0.1);

        // Row of controls for fontFace
        wxStaticText* fontFaceLabel = new wxStaticText(this, wxID_ANY, _("Font Face:"));
 
        std::map<wxString, int> fontFaceMap = {
            { _("Hershey Simplex"), cv::FONT_HERSHEY_SIMPLEX },
            { _("Hershey Plain"), cv::FONT_HERSHEY_PLAIN },
            { _("Hershey Duplex"), cv::FONT_HERSHEY_DUPLEX },
            { _("Hershey Complex"), cv::FONT_HERSHEY_COMPLEX },
            { _("Hershey Triplex"), cv::FONT_HERSHEY_TRIPLEX },
            { _("Hershey Complex Small"), cv::FONT_HERSHEY_COMPLEX_SMALL },
            { _("Hershey Script Simplex"), cv::FONT_HERSHEY_SCRIPT_SIMPLEX },
            { _("Hershey Hershey Script Complex"), cv::FONT_HERSHEY_SCRIPT_COMPLEX },
        };

        m_fontFaceCtrl = new wxComboBox(this, wxID_ANY);
        int initialFontFaceIndex = 0;
        int idx = 0;

        for (const auto& entry: fontFaceMap)
        {
            m_fontFaceCtrl->Append(entry.first, reinterpret_cast<void*>(entry.second));

            if (entry.second == spec.fontFace)
            {
                initialFontFaceIndex = idx;
            }

            idx++;
        }

        m_fontFaceCtrl->Select(initialFontFaceIndex);

        // Row of controls for doBlackBackground
        wxStaticText* doBlackBackgroundLabel = new wxStaticText(this, wxID_ANY, _("Black Background:"));
        m_doBlackBackgroundCtrl = new wxCheckBox(this, wxID_ANY, wxEmptyString);
        m_doBlackBackgroundCtrl->SetValue(m_spec.doBlackBackground);

        // Row of controls for doCaptions
        wxStaticText* doCaptionsLabel = new wxStaticText(this, wxID_ANY, _("Do Captions:"));
        m_doCaptionsCtrl = new wxCheckBox(this, wxID_ANY, wxEmptyString);
        m_doCaptionsCtrl->SetValue(m_spec.doCaptions);

        // OK and Cancel buttons
        wxSizer* buttonsSizer = CreateButtonSizer(wxOK | wxCANCEL);

        gridSizer->Add(colCountLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
        gridSizer->Add(m_colCountCtrl, 1, wxEXPAND);
        gridSizer->Add(imageWidthPxLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
        gridSizer->Add(m_imageWidthPxCtrl, 1, wxEXPAND);
        gridSizer->Add(marginPxLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
        gridSizer->Add(m_marginPxCtrl, 1, wxEXPAND);
        gridSizer->Add(fontScaleLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
        gridSizer->Add(m_fontScaleCtrl, 1, wxEXPAND);
        gridSizer->Add(fontFaceLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
        gridSizer->Add(m_fontFaceCtrl, 1, wxEXPAND);
        gridSizer->Add(doBlackBackgroundLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
        gridSizer->Add(m_doBlackBackgroundCtrl, 1, wxEXPAND);
        gridSizer->Add(doCaptionsLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
        gridSizer->Add(m_doCaptionsCtrl, 1, wxEXPAND);

        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
        mainSizer->Add(gridSizer, 1, wxALL | wxEXPAND, 10);
        mainSizer->Add(buttonsSizer, 0, wxALL | wxEXPAND, 10);

        SetSizer(mainSizer);
        mainSizer->SetSizeHints(this);

        Bind(wxEVT_BUTTON, &CollageSpecDialog::OnOk, this, wxID_OK);
        Bind(wxEVT_BUTTON, &CollageSpecDialog::OnCancel, this, wxID_CANCEL);
    }

    void CollageSpecDialog::OnOk(wxCommandEvent& event)
    {
        m_spec.colCount = m_colCountCtrl->GetValue();
        m_spec.imageWidthPx = m_imageWidthPxCtrl->GetValue();
        m_spec.marginPx = m_marginPxCtrl->GetValue();
        m_spec.fontScale = m_fontScaleCtrl->GetValue();
        m_spec.fontFace = reinterpret_cast<int>(m_fontFaceCtrl->GetClientData(m_fontFaceCtrl->GetSelection()));
        m_spec.doBlackBackground = m_doBlackBackgroundCtrl->GetValue();
        m_spec.doCaptions = m_doCaptionsCtrl->GetValue();

        EndModal(wxID_OK);
    }

    void CollageSpecDialog::OnCancel(wxCommandEvent& event)
    {
        EndModal(wxID_CANCEL);
    }
}
