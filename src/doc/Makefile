## $Id: Makefile,v 1.5 1999/07/19 12:00:59 makholm Exp $
## Authors:  Jens Peter Secher (jpsecher@diku.dk)
##           Henning Makholm (makholm@diku.dk)
## Content:  C-Mix system: Documentation Makefile
##
## Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
## Redistribution and modification are allowed under certain
## terms; see the file COPYING.cmix for details.


## MODIFIABLE SECTION ##

MV  = mv
LATEX	= latex
BIBTEX	= bibtex
MAKEINDEX=makeindex

ORG = cmixII.tex parser.tex sharing.fig \
      bta.tex corec.tex output.tex pgm-pa-bta.fig \
      c2core.tex doc.tex overview.tex phases.fig infra.tex \
      cmix-bib.bib pa.tex separatehead.tex Makefile \
      gegen.tex partstat.tex speclib.tex collapse.tex \
      cg.tex callmode.tex dataflow.tex \
      tables.tex end.tex cmixdoc.cls extfunSE.fig
FIGURES := $(filter %.fig,$(wildcard $(ORG)))
EPSFIGURES := $(patsubst %.fig,%.eps,$(wildcard $(FIGURES)))

TEMPLATE = template.tex

INTER = *.xyc *.aux *.log *.bbl *.blg *.toc *.idx

JUNK = foo foo.* ost ost.* *~

RESULT = *.dvi *.ps

TARFILE = doc

## END OF MODIFIABLE SECTION ##

TEXS = $(filter %.tex,$(ORG))
STYS = $(filter %.sty,$(ORG))
BIBS = $(filter %.bib %.bst,$(ORG))

.PHONY: clean dvi

dvi:	cmixII.dvi

cmixII.dvi: $(EPSFIGURES) $(TEXS) $(STYS) $(BIBS) cmixdoc.cls
	$(LATEX) cmixII
	$(BIBTEX) cmixII
	$(LATEX) cmixII
	$(MAKEINDEX) cmixII
	$(LATEX) cmixII

$(EPSFIGURES): %.eps: %.fig
	fig2dev -Lps $< $@

$(TARFILE).tgz: $(ORG) $(TEMPLATE)
	tar cf $(TARFILE) $^
	gzip $(TARFILE) 
	$(MV) $(TARFILE).gz $@

$(FIGS:.fig=.eps): %.eps: %.fig
	fig2dev -L ps $< >! $@ 

clean:
	$(RM) $(INTER) $(JUNK) $(RESULT)

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.


# IF YOU PUT ANYTHING HERE IT WILL GO AWAY
