/****************************************************************************
 *
 * class CaeUnsThermalDesktop
 *
 * Proprietary software product of Pointwise, Inc.
 * Copyright (c) 1995-2014 Pointwise, Inc.
 * All rights reserved.
 *
 ***************************************************************************/

/*
MSC Nastran 2014 Quick Reference Guide
https://simcompanion.mscsoftware.com/infocenter/index?page=content&id=DOC10654&cat=MSC_NASTRAN_DOCUMENTATION_2014&actp=LIST
*/

#include "apiCAEP.h"
#include "apiCAEPUtils.h"
#include "apiGridModel.h"
#include "apiPWP.h"
#include "runtimeWrite.h"
#include "pwpPlatform.h"

#include "CaePlugin.h"
#include "CaeUnsGridModel.h"
#include "CaeUnsThermalDesktop.h"


<<<<<<< HEAD
static const PWP_UINT32 UnspecifiedId = 2147483647;
static const char *attrWideFormat = "WideFormat";
static const char *attrShellThickness = "ShellThickness";
static const char *attrShellOrientation = "ShellOrientation";
=======
const char *attrWideFormat = "WideFormat";
const char *attrShellThickness = "ShellThickness";
const char *attrShellOrientation = "ShellOrientation";
>>>>>>> origin/master
enum {
    NormalsOut,
    NormalsIn
};


//***************************************************************************
//***************************************************************************
//***************************************************************************

CaeUnsThermalDesktop::CaeUnsThermalDesktop(CAEP_RTITEM *pRti, PWGM_HGRIDMODEL
        model, const CAEP_WRITEINFO *pWriteInfo) :
    CaeUnsPlugin(pRti, model, pWriteInfo),
    saveOF_(0),
    wideFormat_(true),
    defThickness_(0.0),
    defOrient_(),
    fmtMat1_(0),
    fmtSolid_(0),
    fmtShell_(0),
    fmtGrid_(0),
    fmtTri_(0),
    fmtQuad_(0),
    fmtTet_(0),
    fmtPyr_(0),
    fmtPri_(0),
    fmtHex_(0),
    totalElemCnt_(0)
{
}


CaeUnsThermalDesktop::~CaeUnsThermalDesktop()
{
}


bool
CaeUnsThermalDesktop::beginExport()
{
#if defined(WINDOWS)
    saveOF_ = _set_output_format(_TWO_DIGIT_EXPONENT);
#endif

    model_.getAttribute(attrWideFormat, wideFormat_);
    model_.getAttribute(attrShellThickness, defThickness_);
    model_.getAttribute(attrShellOrientation, defOrient_);

    if (wideFormat_) {
        fmtMat1_ = "MAT1    %8d%8g%8g%8g%8g%8g%8g%8g\n";
        fmtSolid_ = "PSOLID  %8d%8d\n";
        fmtShell_ = "PSHELL  %8d%8d%8g                                    0.00\n";

        fmtGrid_ = "GRID*   %16d                %16.9e%16.9e*GRID000\n"
                   "*GRID000%16.9e\n";

        fmtTri_  = "CTRIA3  %8d%8d%8d%8d%8d            0.00\n";
        fmtQuad_ = "CQUAD4  %8d%8d%8d%8d%8d%8d            0.00\n";
        fmtTet_  = "CTETRA  %8d%8d%8d%8d%8d%8d\n";
        fmtPyr_  = "CPYRAM  %8d%8d%8d%8d%8d%8d%8d\n";
        fmtPri_  = "CPENTA  %8d%8d%8d%8d%8d%8d%8d%8d\n";
        fmtHex_  = "CHEXA   %8d%8d%8d%8d%8d%8d%8d%8d\n"
                   "        %8d%8d\n";
    }
    else {
        fmtMat1_ = "MAT1    %d,%g,%g,%g,%g,%g,%g,%g\n";
        fmtSolid_ = "PSOLID,%7d,%d\n";
        fmtShell_ = "PSHELL,%7d,%d,%g,,,,,0\n";

        fmtGrid_ = "GRID  ,%7d,,%16.9e,%16.9e,%16.9e\n";

        fmtTri_  = "CTRIA3,%7d,%d,%7d,%7d,%7d,,0.000000\n";
        fmtQuad_ = "CQUAD4,%7d,%d,%7d,%7d,%7d,%7d,,0.000000\n";
        fmtTet_  = "CTETRA,%7d,%d,%7d,%7d,%7d,%7d,\n";
        fmtPyr_  = "CPYRAM,%7d,%d,%7d,%7d,%7d,%7d,%7d,\n";
        fmtPri_  = "CPENTA,%7d,%d,%7d,%7d,%7d,%7d,%7d,%7d,\n";
        fmtHex_  = "CHEXA ,%7d,%d,%7d,%7d,%7d,%7d,%7d,%7d,\n"
                   "      ,%7d,%7d\n";
    }
    // vertices + elements
    setProgressMajorSteps(2);
    return true;
}

PWP_BOOL
CaeUnsThermalDesktop::write()
{
    return writeHeader() && writeVertices() && writeElements();
}

bool
CaeUnsThermalDesktop::endExport()
{
#if defined(WINDOWS)
    _set_output_format(saveOF_);
#endif
    return true;
}


bool
CaeUnsThermalDesktop::writeHeader()
{
    bool ret = true;

    writeSectionComment("Created by Pointwise");

    const double E = 3.47;
    const double G = 0.0;
    const double NU = 0.0;
    const double RHO = 4.28;
    const double A = 6.5e-6;
    const double TREF = 537.0;
    const double GE = 0.23;

    PWGM_CONDDATA cond;
    int id = 0;
    int matId = 0;
    CaeUnsBlock blk(model_);
    CaeUnsPatch patch(model_);

    writeSectionComment("Solid Materials");
    while (blk.isValid()) {
        if (!blk.condition(cond)) {
            ret = false;
            break;
        }
        // Index by the cond name ptr. Ptr stays the same for a given run.
        matId = matMap_[cond.name];
        if (0 == matId) {
            // first time finding this vc name
            matId = (int)matMap_.size();
            matMap_[cond.name] = matId;
            if (0 > fprintf(fp(), fmtMat1_, matId, E, G, NU, RHO, A, TREF, GE)) {
                ret = false;
                break;
            }
        }
        ++blk;
    }

    if (ret) {
        writeSectionComment("Shell Materials");
        while (patch.isValid()) {
            if (!patch.condition(cond)) {
                ret = false;
                break;
            }
            // Index by the cond name ptr. Ptr stays the same for a given run.
            matId = matMap_[cond.name];
            if (0 == matId) {
                // first time finding this bc name
                matId = (int)matMap_.size();
                matMap_[cond.name] = matId;
                if (0 > fprintf(fp(), fmtMat1_, matId, E, G, NU, RHO, A, TREF, GE)) {
                    ret = false;
                    break;
                }
            }
            ++patch;
        }
    }

    totalElemCnt_ = 0;

    if (ret) {
        writeSectionComment("Solids");
        blk.moveFirst(model_);
        while (blk.isValid()) {
            if (!blk.condition(cond)) {
                ret = false;
                break;
            }
            writeComment("PSOLID_NAME = \"", cond.name, "\"");
            matId = matMap_[cond.name];
            if (0 > fprintf(fp(), fmtSolid_, ++id, matId)) {
                ret = false;
                break;
            }
            totalElemCnt_ += blk.elementCount();
            ++blk;
        }
    }

    if (ret) {
        writeSectionComment("Shells");
        patch.moveFirst(model_);
        while (patch.isValid()) {
            if (!patch.condition(cond)) {
                ret = false;
                break;
            }
            // Index by the cond name ptr. Ptr stays the same for a given run.
            matId = matMap_[cond.name];
            writeComment("PSHELL_NAME = \"", cond.name, "\"");
            if (0 > fprintf(fp(), fmtShell_, ++id, matId, defThickness_)) {
                ret = false;
                break;
            }
            totalElemCnt_ += patch.elementCount();
            ++patch;
        }
    }

    return ret;
}


bool
CaeUnsThermalDesktop::writeVertices()
{
    bool ret = progressBeginStep(model_.vertexCount());
    writeSectionComment("Grid Vertices");
    int id = 0;
    CaeUnsVertex v(model_);
    while (ret && v.isValid()) {
        if (0 > fprintf(fp(), fmtGrid_, ++id, v.x(), v.y(), v.z())) {
            ret = false;
            break;
        }
        ret = progressIncrement();
        ++v;
    }
    return progressEndStep() && ret;
}


bool
CaeUnsThermalDesktop::writeElements()
{
    bool ret = progressBeginStep(totalElemCnt_) &&
        model_.appendEnumElementOrder(PWGM_ELEMORDER_VC);
    PWGM_CONDDATA cond = { 0 };
    int setId = 0; // SET3 id
    int matId = 0; // material id
    int elemId = 0; // current SET3 element id
    int fromElemId = 0; // first elem id in current SET3
    if (ret) {
        PWP_UINT32 blkId = PWP_BADID;
        PWGM_ENUMELEMDATA data;

        writeSectionComment("Volume Elements");
        CaeUnsElement elem(model_);
        while (progressIncrement() && elem.isValid()) {
            if (!elem.data(data)) {
                ret = false;
                break;
            }
            if (PWGM_HELEMENT_PID(data.hBlkElement) != blkId) {
                // New block, check if VC has changed
                blkId = PWGM_HELEMENT_PID(data.hBlkElement);
                CaeUnsBlock blk(model_, blkId);
                PWGM_CONDDATA blkCond;
                if (!blk.condition(blkCond)) {
                    ret = false;
                    break;
                }
                if (blkCond.name != cond.name) {
                    // End previous element material group.
                    if ((0 == fromElemId) || (UnspecifiedId == cond.id)) {
                        // do nothing
                    }
                    else if (!writeElemSet(cond.name, ++setId, fromElemId, elemId)) {
                        ret = false;
                        break;
                    }
                    cond = blkCond;
                    matId = matMap_[cond.name];
                    writeComment(" BEGIN PSOLID \"", cond.name, "\"");
                    fromElemId = elemId + 1;
                }
            }
            if (!writeElement(data.elemData, ++elemId, matId)) {
                ret = false;
                break;
            }
            ++elem;
        }
        // end the last material group.
        if (!ret || (0 == fromElemId) || (UnspecifiedId == cond.id)) {
            // do nothing
        }
        else if (!writeElemSet(cond.name, ++setId, fromElemId, elemId)) {
            ret = false;
        }
    }

    writeSectionComment("Boundary Elements");
    CaeUnsPatch patch(model_);
    while (ret && patch.isValid()) {
        if (!patch.condition(cond)) {
            ret = false;
            break;
        }
        matId = matMap_[cond.name];
        writeComment(" BEGIN PSHELL \"", cond.name, "\"");
        fromElemId = elemId + 1;
        CaeUnsElement elem(patch);
        while (elem.isValid()) {
            if (!writeElement(elem, ++elemId, matId) || !progressIncrement()) {
                ret = false;
                break;
            }
            ++elem;
        }
        if (!ret || (UnspecifiedId == cond.id)) {
            // do nothing
        }
        else if (!writeElemSet(cond.name, ++setId, fromElemId, elemId)) {
            ret = false;
        }
        ++patch;
    }
    return progressEndStep() && ret;
}


#define LU(v)   (unsigned long)(v + 1)

bool
CaeUnsThermalDesktop::writeElement(const PWGM_ELEMDATA &data, int id, int mat)
{
    bool ret = true;
    switch (data.type) {
    case PWGM_ELEMTYPE_BAR:
        fprintf(fp(), "$BAR %d %lu %lu\n", id, LU(data.index[0]),
            LU(data.index[1]));
        break;
    case PWGM_ELEMTYPE_HEX:
        fprintf(fp(), fmtHex_, id, mat, LU(data.index[0]),
            LU(data.index[1]), LU(data.index[2]), LU(data.index[3]),
            LU(data.index[4]), LU(data.index[5]), LU(data.index[6]),
            LU(data.index[7]));
        break;
    case PWGM_ELEMTYPE_QUAD:
        if (NormalsIn == defOrient_) {
            fprintf(fp(), fmtQuad_, id, mat, LU(data.index[0]),
                LU(data.index[1]), LU(data.index[2]), LU(data.index[3]));
        }
        else {
            fprintf(fp(), fmtQuad_, id, mat, LU(data.index[3]),
                LU(data.index[2]), LU(data.index[1]), LU(data.index[0]));
        }
        break;
    case PWGM_ELEMTYPE_TRI:
        if (NormalsIn == defOrient_) {
            fprintf(fp(), fmtTri_, id, mat, LU(data.index[0]),
                LU(data.index[1]), LU(data.index[2]));
        }
        else {
            fprintf(fp(), fmtTri_, id, mat, LU(data.index[2]),
                LU(data.index[1]), LU(data.index[0]));
        }
        break;
    case PWGM_ELEMTYPE_TET:
        fprintf(fp(), fmtTet_, id, mat, LU(data.index[0]),
            LU(data.index[1]), LU(data.index[2]), LU(data.index[3]));
        break;
    case PWGM_ELEMTYPE_WEDGE:
        fprintf(fp(), fmtPri_, id, mat, LU(data.index[0]),
            LU(data.index[1]), LU(data.index[2]), LU(data.index[3]),
            LU(data.index[4]), LU(data.index[5]));
        break;
    case PWGM_ELEMTYPE_PYRAMID:
        fprintf(fp(), fmtPyr_, id, mat, LU(data.index[0]),
            LU(data.index[1]), LU(data.index[2]), LU(data.index[3]),
            LU(data.index[4]));
        break;
    case PWGM_ELEMTYPE_POINT:
        fprintf(fp(), "$POINT %lu\n", LU(data.index[0]));
        break;
    default:
        ret = false;
        break;
    }
    return ret;
}


bool
CaeUnsThermalDesktop::writeElemSet(const char *name, int setId, int fromElemId,
    int toElemId)
{
    // $SET3_NAME = "The Name"
    // SET3 33 POINT 20 THRU 60
    bool ret = true;
    fprintf(fp(), "$SET3_NAME = \"%s\"\n", name);
    fprintf(fp(), "%-8.8s%-8d%-8.8s%-8d%-8.8s%-8d\n", "SET3", setId, "ELEM",
        fromElemId, "THRU", toElemId);
    fputs("$\n", fp());
    return ret;
}


void
CaeUnsThermalDesktop::writeComment(const char *pfx, const char *text,
    const char *sfx)
{
    fprintf(fp(), "$%s%s%s\n", (pfx ? pfx : ""), (text ? text : ""),
        (sfx ? sfx : ""));
}


void
CaeUnsThermalDesktop::writeComment(const char *pfx, const char *text)
{
    writeComment(pfx, text, "");
}


void
CaeUnsThermalDesktop::writeComment(const char *text)
{
    writeComment("", text, "");
}


void
CaeUnsThermalDesktop::writeSectionComment(const char *text)
{
    const char * const hr = "--------------------------------------------";
    writeComment(hr);
    writeComment(" ", text);
    writeComment(hr);
}


//===========================================================================
// face streaming handlers
//===========================================================================

PWP_UINT32
CaeUnsThermalDesktop::streamBegin(const PWGM_BEGINSTREAM_DATA &data)
{
    char msg[128];
    sprintf(msg, "STREAM BEGIN: %lu", (unsigned long)data.totalNumFaces);
    sendInfoMsg(msg);
    return 1;
}

PWP_UINT32
CaeUnsThermalDesktop::streamFace(const PWGM_FACESTREAM_DATA &data)
{
    char msg[128];
    sprintf(msg, "  STREAM FACE: %lu %lu", (unsigned long)data.owner.cellIndex,
        (unsigned long)data.face);
    sendInfoMsg(msg);
    return 1;
}

PWP_UINT32
CaeUnsThermalDesktop::streamEnd(const PWGM_ENDSTREAM_DATA &data)
{
    char msg[128];
    sprintf(msg, "STREAM END: %s", (data.ok ? "true" : "false"));
    sendInfoMsg(msg);
    return 1;
}


//===========================================================================
// called ONCE when plugin first loaded into memory
//===========================================================================

bool
CaeUnsThermalDesktop::create(CAEP_RTITEM &rti)
{
    bool ret = publishBoolValueDef(rti, attrWideFormat, true,
            "Export using wide column format") &&
         publishRealValueDef(rti, attrShellThickness, 0.0,
            "Default PSHELL Thickness", 0.0, 9.0e12) &&
         publishEnumValueDef(rti, attrShellOrientation, "NormalsOut",
              "Default PSHELL orientation", "NormalsOut|NormalsIn");
    return ret;
}


//===========================================================================
// called ONCE just before plugin unloaded from memory
//===========================================================================

void
CaeUnsThermalDesktop::destroy(CAEP_RTITEM &rti)
{
    (void)rti.BCCnt; // silence unused arg warning
}
