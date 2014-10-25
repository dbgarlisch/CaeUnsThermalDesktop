/****************************************************************************
 *
 * class CaeUnsThermalDesktop
 *
 * Proprietary software product of Pointwise, Inc.
 * Copyright (c) 1995-2014 Pointwise, Inc.
 * All rights reserved.
 *
 ***************************************************************************/

#include "apiCAEP.h"
#include "apiCAEPUtils.h"
#include "apiGridModel.h"
#include "apiPWP.h"
#include "runtimeWrite.h"
#include "pwpPlatform.h"

#include "CaePlugin.h"
#include "CaeUnsGridModel.h"
#include "CaeUnsThermalDesktop.h"


const char *attrWideFormat = "wideFormat";
const char *attrShellThickness = "ShellThickness";
const char *attrShellOrientation = "ShellOrientation";
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
    header_(0),
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
        header_ = "MAT1           1                            4.28 "
                  "6.5e-006  537.00    0.23\n";
        fmtSolid_ = "PSOLID  %8d       1\n";
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
        header_ = "MAT1  ,      1,3.47,,,4.28,6.5e-006,537,0.23,\n"
                  "      ,       ,,,\n";
        fmtSolid_ = "PSOLID,%7d,1\n";
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
    fputs(header_, fp());

    totalElemCnt_ = 0;

    PWGM_CONDDATA cond;
    int id = 0;
    CaeUnsBlock blk(model_);
    while (blk.isValid()) {
        if (!blk.condition(cond)) {
            ret = false;
            break;
        }
        writeComment("PSOLID_NAME = \"", cond.name, "\"");
        if (0 > fprintf(fp(), fmtSolid_, ++id)) {
            ret = false;
            break;
        }
        totalElemCnt_ += blk.elementCount();
        ++blk;
    }

    if (ret) {
        const int MatId = 1;
        CaeUnsPatch patch(model_);
        while (patch.isValid()) {
            if (!patch.condition(cond)) {
                ret = false;
                break;
            }
            writeComment("PSHELL_NAME = \"", cond.name, "\"");
            if (0 > fprintf(fp(), fmtShell_, ++id, MatId, defThickness_)) {
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
    const PWP_UINT32 UnspecifiedId = 2147483647;
    bool ret = progressBeginStep(totalElemCnt_);

    int groupId = 1; // solid/shell seq id
    int elemId = 0; // volume/surface seq id
    int fromElemId = 0; // first elem id in a group

    writeSectionComment("Volume Elements");
    PWGM_CONDDATA cond;
    CaeUnsBlock blk(model_);
    while (ret && blk.isValid()) {
        if (!blk.condition(cond)) {
            ret = false;
            break;
        }
        writeComment(" BEGIN PSOLID \"", cond.name, "\"");
        fromElemId = elemId + 1;
        CaeUnsElement elem(blk);
        while (elem.isValid()) {
            if (!writeElement(elem, ++elemId, groupId) ||
                !progressIncrement()) {
                ret = false;
                break;
            }
            ++elem;
        }
        if (ret && (UnspecifiedId != cond.id)) {
            ret = writeElemSet(cond.name, int(cond.id), fromElemId, elemId);
        }
        ++blk;
        ++groupId;
    }

    writeSectionComment("Boundary Elements");
    CaeUnsPatch patch(model_);
    while (ret && patch.isValid()) {
        if (!patch.condition(cond)) {
            ret = false;
            break;
        }
        writeComment(" BEGIN PSHELL \"", cond.name, "\"");
        fromElemId = elemId + 1;
        CaeUnsElement elem(patch);
        while (elem.isValid()) {
            if (!writeElement(elem, ++elemId, groupId) ||
                !progressIncrement()) {
                ret = false;
                break;
            }
            ++elem;
        }
        if (ret && (UnspecifiedId != cond.id)) {
            ret = writeElemSet(cond.name, int(cond.id), fromElemId, elemId);
        }
        ++patch;
        ++groupId;
    }

    return progressEndStep() && ret;
}


#define LU(v)   (unsigned long)(v + 1)

bool
CaeUnsThermalDesktop::writeElement(const CaeUnsElement &elem, int id, int grp)
{
    PWGM_ELEMDATA data;
    bool ret = elem.data(data);
    if (ret) {
        switch (data.type) {
        case PWGM_ELEMTYPE_BAR:
            fprintf(fp(), "$BAR %d %lu %lu\n", id, LU(data.index[0]),
                LU(data.index[1]));
            break;
        case PWGM_ELEMTYPE_HEX:
            fprintf(fp(), fmtHex_, id, grp, LU(data.index[0]),
                LU(data.index[1]), LU(data.index[2]), LU(data.index[3]),
                LU(data.index[4]), LU(data.index[5]), LU(data.index[6]),
                LU(data.index[7]));
            break;
        case PWGM_ELEMTYPE_QUAD:
            if (NormalsIn == defOrient_) {
                fprintf(fp(), fmtQuad_, id, grp, LU(data.index[0]),
                    LU(data.index[1]), LU(data.index[2]), LU(data.index[3]));
            }
            else {
                fprintf(fp(), fmtQuad_, id, grp, LU(data.index[3]),
                    LU(data.index[2]), LU(data.index[1]), LU(data.index[0]));
            }
            break;
        case PWGM_ELEMTYPE_TRI:
            if (NormalsIn == defOrient_) {
                fprintf(fp(), fmtTri_, id, grp, LU(data.index[0]),
                    LU(data.index[1]), LU(data.index[2]));
            }
            else {
                fprintf(fp(), fmtTri_, id, grp, LU(data.index[2]),
                    LU(data.index[1]), LU(data.index[0]));
            }
            break;
        case PWGM_ELEMTYPE_TET:
            fprintf(fp(), fmtTet_, id, grp, LU(data.index[0]),
                LU(data.index[1]), LU(data.index[2]), LU(data.index[3]));
            break;
        case PWGM_ELEMTYPE_WEDGE:
            fprintf(fp(), fmtPri_, id, grp, LU(data.index[0]),
                LU(data.index[1]), LU(data.index[2]), LU(data.index[3]),
                LU(data.index[4]), LU(data.index[5]));
            break;
        case PWGM_ELEMTYPE_PYRAMID:
            fprintf(fp(), fmtPyr_, id, grp, LU(data.index[0]),
                LU(data.index[1]), LU(data.index[2]), LU(data.index[3]),
                LU(data.index[4]));
            break;
        case PWGM_ELEMTYPE_POINT:
            fprintf(fp(), "$POINT %lu\n", LU(data.index[0]));
            break;
        }
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
