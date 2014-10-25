/****************************************************************************
 *
 * class CaeUnsThermalDesktop
 *
 * Proprietary software product of Pointwise, Inc.
 * Copyright (c) 1995-2014 Pointwise, Inc.
 * All rights reserved.
 *
 ***************************************************************************/

#ifndef _CAEUNSTHERMALDESKTOP_H_
#define _CAEUNSTHERMALDESKTOP_H_

#include "apiGridModel.h"
#include "apiPWP.h"

#include "CaePlugin.h"
#include "CaeUnsGridModel.h"


//***************************************************************************
//***************************************************************************
//***************************************************************************

class CaeUnsThermalDesktop :
    public CaeUnsPlugin,
    public CaeFaceStreamHandler {

public:
    CaeUnsThermalDesktop(CAEP_RTITEM *pRti, PWGM_HGRIDMODEL model,
        const CAEP_WRITEINFO *pWriteInfo);
    virtual ~CaeUnsThermalDesktop();
    static bool create(CAEP_RTITEM &rti);
    static void destroy(CAEP_RTITEM &rti);

private:

    virtual bool        beginExport();
    virtual PWP_BOOL    write();
    virtual bool        endExport();

    bool    writeHeader();
    bool    writeVertices();
    bool    writeElements();
    bool    writeElement(const CaeUnsElement &elem, int id, int grp);
    bool    writeElemSet(const char *name, int setId, int fromElemId,
                int toElemId);

    void    writeComment(const char *pfx, const char *text, const char *sfx);
    void    writeComment(const char *pfx, const char *text);
    void    writeComment(const char *text);
    void    writeSectionComment(const char *text);

    // face streaming handlers
    virtual PWP_UINT32 streamBegin(const PWGM_BEGINSTREAM_DATA &data);
    virtual PWP_UINT32 streamFace(const PWGM_FACESTREAM_DATA &data);
    virtual PWP_UINT32 streamEnd(const PWGM_ENDSTREAM_DATA &data);

private:

    unsigned int    saveOF_;
    bool            wideFormat_;
    PWP_REAL        defThickness_;
    PWP_UINT32      defOrient_;
    const char *    header_;
    const char *    fmtSolid_;
    const char *    fmtShell_;
    const char *    fmtGrid_;
    const char *    fmtTri_;
    const char *    fmtQuad_;
    const char *    fmtTet_;
    const char *    fmtPyr_;
    const char *    fmtPri_;
    const char *    fmtHex_;
    PWP_UINT32      totalElemCnt_;
};

#endif // _CAEUNSTHERMALDESKTOP_H_
