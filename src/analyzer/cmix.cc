/* Authors:  Henning Makholm (makholm@diku.dk)
 *           Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 * Content:  C-Mix system: Main program
 *
 * Copyright © 1997-2000. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "options.h"
#include "commonout.h"
#include "fileops.h"
#include "parser.h"
#include "c2core.h"
#include "outcore.h"
#include "directives.h"
#include "analyses.h"
#include "outanno.h"
#include "renamer.h"

void banner(ostream &ost) {
    static bool alreadydone = false ;
    if ( !alreadydone )
        ost << "This is C-Mix/II, release " << CmixRelease << endl
            << "© 1993-2000 the TOPPS group at DIKU, U of Copenhagen" << endl ;
    alreadydone = true ;
}

void handleNonOption(const char *cp) {
    size_t length = strlen(cp) ;
    if ( length >= 3 && 0 == strcmp(cp + length - 2 , ".c") ) {
        char const *const ignoredotc[] = { ".c", NULL };
        CmixOutput::ProposeBase(cp,100,ignoredotc);
        directives.add_source(cp,Position()) ;
    } else {
        // Take it as the name of a script file
        char const *const ignoredotcmx[] = { ".cmx", "", NULL };
        CmixOutput::ProposeBase(cp,200,ignoredotcmx);
        if ( cmxLexWrapper(cp,Position()).parse() &&
             TheCmxResult.status != CmxResult::GotDirectives )
            Diagnostic(ERROR,Position()) << cp
                                  << " should be a script file but is not";
    }
}

// Used to hold the transformed program.
static CProgram* full_c_pgm;
static C_Pgm core_c_pgm;
static BTresult bt_result ;
static CoreAnnotator *coreanno ;

static void
dumpAnnoFile(const char* ext, const char* type,bool force_core = true)
{
  assert(ext != NULL && type != NULL);
  CmixOutput annofile(ext);
  OutputContainer oc(OProgram, allAttributes(), "cmixII");
  if ( annofile ) {
    if ( !quiet_mode ) cout << "Generating " << type << endl ;
    if ( force_core || core_mode ) {
      NameMap namemap ;
      MakeUniqueNames(core_c_pgm,bt_result,namemap,NULL);
      Outcore outcore(oc,*coreanno,OProgram);
      oc.add(outcore(core_c_pgm,namemap),OProgram);
    }
    else
      full_c_pgm->output(oc,bt_result);
    if ( quiet_mode <= 1 ) cout << "Writing to "
                                << annofile.GetFilename() << endl ;
    oc.do_export(annofile);
  }
  else
    Diagnostic(FATAL,Position()) << "couldn't write annotated program to "
                                 << annofile.GetFilename();
}

int
main(int argc,char **argv) {
    parse_options(argc,argv);
    Diagnostic::EnterNewPhase();

    if ( quiet_mode <= 1 ) banner(cout) ;
    NoCoreAnno noanno ;
    coreanno = &noanno ;

    directives.ReadSubjectProgram();
    Diagnostic::EnterNewPhase();
    if ( only_preprocess_mode )
        return 0 ;
    full_c_pgm = parser.get_program();

    if ( !quiet_mode ) cout << "Applying user annotations" << endl ;
    ApplyUserAnnotations(full_c_pgm);
    Diagnostic::EnterNewPhase();
    
    if ( !quiet_mode ) cout << "Type checking subject program" << endl ;
    check_program(full_c_pgm);
    Diagnostic::EnterNewPhase();

    if ( !quiet_mode ) cout << "Translating to core C" << endl ;
    c2core(*full_c_pgm,core_c_pgm);
    foreach(i,directives.generators,Plist<GeneratorDirective>)
        i->g2core(core_c_pgm);
    // use a separate loop to delete the generator directive:
    // g2core may need to refer to earlier directives to
    // create error messages.
    foreach(i,directives.generators,Plist<GeneratorDirective>)
        delete *i ;
    if ( DumpCoreC ) dumpAnnoFile(".core","core c output");
    Diagnostic::EnterNewPhase();

    if ( !quiet_mode ) cout << "Doing pointer analysis" << endl ;
    PAresult pa_result ;
    pointsToAnalysis(core_c_pgm,pa_result);
    coreanno = new OutPa(pa_result,coreanno);
    if ( DumpPA ) dumpAnnoFile(".pa","PA output");
    Diagnostic::EnterNewPhase();

    if ( !quiet_mode ) cout << "Evacuating complex initializers" << endl ;
    SubstInitializers(core_c_pgm,pa_result);
    Diagnostic::EnterNewPhase();
    
    if ( !quiet_mode ) cout << "Computing call graph" << endl ;
    CGresult cg_result ;
    mkCallGraph(core_c_pgm,pa_result,cg_result);
    coreanno = new OutCg(cg_result,coreanno);
    Diagnostic::EnterNewPhase();

    if ( !quiet_mode ) cout << "Computing truly locals" << endl ;
    ALocSet uniq_result ;
    TrulyLocal(core_c_pgm,cg_result,pa_result,uniq_result);
    coreanno = new OutTrulyLocal(pa_result,coreanno);
    Diagnostic::EnterNewPhase();

    if ( !quiet_mode ) cout << "Doing binding-time analysis" << endl ;
    // bt_result is a global
    binding_time_analysis(core_c_pgm,pa_result,bt_result);
    coreanno = new OutBt(bt_result,coreanno);
    if ( DumpBta ) dumpAnnoFile(".bta","raw binding-time annotated output");
    Diagnostic::EnterNewPhase();
    
    if ( !quiet_mode ) cout << "Separating structures" << endl ;
    Separate(core_c_pgm,bt_result);
    Diagnostic::EnterNewPhase();    

    if ( !quiet_mode ) cout << "Doing code-sharing analysis" << endl ;
    SAresult sa_result ;
    findUnsharable(core_c_pgm,pa_result,cg_result,bt_result,sa_result);
    coreanno = new OutSa(sa_result,coreanno);
    Diagnostic::EnterNewPhase();

    if ( !quiet_mode ) cout << "Checking binding-time correctness" << endl ;
    checkUserAnnoSanity(core_c_pgm,bt_result);
    Diagnostic::EnterNewPhase();

    // split phase will go here...

    DFresult df_result ;
    if ( !quiet_mode ) cout << "Doing data-flow analyses" << endl ;
    df_result.Analyse(core_c_pgm,pa_result,uniq_result);
    coreanno = new OutDf(df_result,coreanno);
    Diagnostic::EnterNewPhase();

    if ( !quiet_mode ) cout << "Sorting structure declarations" << endl ;
    sortStructDecls(core_c_pgm);
    Diagnostic::EnterNewPhase();
    
    dumpAnnoFile(".ann","binding-time annotated output",false);
    
    CmixOutput pgenfile("-gen.c");
    if ( quiet_mode <= 1 ) cout << "Writing to "
                                << pgenfile.GetFilename() << endl ;
    if ( pgenfile )
        emitGeneratingExtension(core_c_pgm,bt_result,pa_result,
                                sa_result,df_result,
                                pgenfile);
    if ( !pgenfile )
        Diagnostic(FATAL,Position()) << "couldn't write generating extension "
                                     << "to " << pgenfile.GetFilename() ;
    Diagnostic::EnterNewPhase();

    return 0 ;
}

