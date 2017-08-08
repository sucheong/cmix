% Authors:  Jens Peter Secher (jpsecher@diku.dk)
%           Arne Glenstrup (panic@diku.dk)
%           Henning Makholm (makholm@diku.dk)
% Content:  C-Mix system: documentation class for \LaTeX
%
% Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
% Redistribution and modification are allowed under certain
% terms; see the file COPYING.cmix for details.

\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{cmixdoc}[1998/11/17 Documentation for cmix]

\DeclareOption*{\PassOptionsToClass{\CurrentOption}{article}}
\ProcessOptions\relax
\LoadClass{article}

\RequirePackage[matrix,arrow,frame,curve]{xy}%\CompileMatrices
\RequirePackage{anysize}        % widens the margins excessively
\RequirePackage{latexsym}       % for \unrhd
\RequirePackage{amssymb}        % used for pseudocode symbols in c2core
\RequirePackage{alltt}          % used by various examples
\RequirePackage{theorem}        % lets us select upright font for thm envs
\RequirePackage{varioref}       % for \vref
\RequirePackage{index}          % for \shortindexingon
\RequirePackage{xspace}         % used in the \cmix, etc. macros
\RequirePackage{epsfig}         % used to include figures
\RequirePackage{shortvrb}       % for \MakeShortVerb
\RequirePackage{mathligs}       % widely used
\RequirePackage{semantic}       % ditto
\RequirePackage{apacite}        % for readable citings
\bibliographystyle{apacite}

\errorcontextlines 400

%% British/American
\newcommand{\analys}{analyz}
\newcommand{\generalis}{generaliz}
\newcommand{\Generalis}{Generaliz}
\newcommand{\specialis}{specializ}
\newcommand{\Specialis}{Specializ}
\newcommand{\optimis}{optimiz}
\newcommand{\revolutionis}{revolutioniz}
\newcommand{\utilis}{utiliz}
\newcommand{\initialis}{initializ}
\newcommand{\characteris}{characteriz}
\newcommand{\realis}{realiz}
\newcommand{\normalis}{normaliz}
\newcommand{\formalis}{formaliz}
\newcommand{\recognis}{recogniz}
\newcommand{\flavour}{flavor}
\newcommand{\favour}{favor}
\newcommand{\colour}{color}
\newcommand{\behaviour}{behavior}
\newcommand{\amortis}{amortiz}
\newcommand{\emphasis}{emphasiz}
\newcommand{\stabilis}{stabiliz}
\newcommand{\Stabilis}{Stabiliz}
\newcommand{\labelled}{labeled}
\newcommand{\memois}{memoiz}
\newcommand{\Memois}{Memoiz}
\newcommand{\externalis}{externaliz}
%\newcommand{\s}{z}

%%%%%%%%%%%%%%%%%%%  DECLARATIONS %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% brand names etc.
%\newcommand{\cmix}{C-Mix$_{I\!I}$\xspace}
\def\diagminuskern#1#2{{%
        \dimen0#2\relax
        \count0=\wd#1%
        \multiply\count0by100
        \divide\count0by\ht#1%
        \multiply\dimen0by\count0
        \kern-.01\dimen0}}
\DeclareRobustCommand{\cmix}{{\upshape
        C%
        \setbox0\hbox{C}%
        \dimen0\ht0
        \advance\dimen0by-.13ex
        \kern-.1ex
        \raise.5\dimen0\hbox{\vrule height.13ex width.5ex}%
        \kern-.05ex
        Mix%
        \setbox0\hbox{\setbox0\hbox{/}\raise\dp0\box0}%
        \dimen0=.1\wd0                    % line width in 'II' part
        \dimen2=\ht0\advance\dimen2by-1ex % descender length for slash
        \ifdim\dimen2>1ex\relax\dimen2=1ex\fi
        \diagminuskern0{\dimen2}%
        \raise-\dimen2\copy0
        \dimen4\ht0\advance\dimen4by-\dimen2
        \dimen2=.6ex
        \advance\dimen4by-\dimen2
        \diagminuskern0{\dimen4}\kern-\dimen0
        \dimen4=\dimen2\advance\dimen4by-\dimen0
        \raise-\dimen2\vbox{\hrule height\dimen0\hbox{%
                \hskip.5\dimen4
                \vrule width\dimen0 height\dimen4 depth\dimen4
                \hskip.5\dimen4
                \vrule width\dimen0
                \hskip.5\dimen4
                }\hrule height\dimen0}}\xspace}
\newcommand{\ansiC}{ISO C\xspace}
\newcommand{\coreC}{Core C\xspace}

% Define nice \html-command
\let\@savedslash=/
\newcommand\@slash{\discretionary{/}{}{/}}
\newcommand\@url[1]{\textsf{#1}\egroup}
\def\makenormalslash{\catcode`\/=12}
{\catcode`\/=\active
\gdef\makediscretionaryslash{\catcode`\/=\active\def/{\@slash}}
\gdef\URL{\bgroup\catcode`\/=\active
\def~{\~{}}\def\do##1{\catcode`##1=11}\do\~\do\_
\makediscretionaryslash\@url}}

% Shorthands macros

\newcommand{\textfnam}{\texttt}

\renewcommand{\predicate}[1]{$ #1 $}

\newcommand\mc{\multicolumn}

\newcommand{\x}{\times}

\newcommand{\append}{\mathbin{+\mkern-10mu+}}

\newcommand{\eg}{\emph{e.g.}\xspace}

\newcommand{\ie}{\emph{i.e.}\xspace}

\newcommand{\etc}{etc.\xspace}

\newcommand{\cons}[1]{\textsf{#1}} % Constructor

\newcommand{\syntax}[1]{\texttt{#1}}

\newcommand{\avec}[2][n]{% arg1: upper limit, arg2: vector name
        \ensuremath{#2_1\dots#2_{#1}}}

\newcommand{\grule}[1]{\texttt{#1}}

\newcommand{\alst}[2][n]{% arg1: upper limit, arg2: elem name
        \ensuremath{#2_1,\dots,#2_{#1}}}

\newcommand{\btT}[2]{\langle{#1}\rangle^{#2}}  
\newcommand{\bt}[1]{\ensuremath{\mbox{bt}(#1)}}

\newcommand{\cnv}[1]
  {\ensuremath{
      {\mathbf (\!|} #1 {\mathbf |\!)}%
      }%
  }
\newcommand{\letin}[2]
  {\ensuremath\mbox{let}
    \begin{array}[t]{lcl}
      #1
    \end{array}\\
    &&\mbox{in } #2
  }

% Generating syntax and backquoting
\newcommand{\gegen}{\@ifstar{\gegeninit}{\gegenx}}
\newcommand{\gegenx}[1]{\ensuremath{\langle}#1\ensuremath{\rangle}}
\newcommand{\gegeninit}[1]
{\ensuremath{\langle\mskip-4mu\langle}#1%
  \ensuremath{\rangle\mskip-4mu\rangle}}
\newcommand{\encode}[1]{\ensuremath{\lfloor}#1\ensuremath{\rfloor}}
\newcommand{\emit}{\@ifstar{\emitdecl}{\emitstmt}}
\newcommand{\emitstmtbegin}{\ensuremath{\lceil}}
\newcommand{\emitstmtend}{\ensuremath{\rceil}}
\newcommand{\emitdeclbegin}{\lower .3ex
  \hbox to 0pt{$\lceil$\hss}\ensuremath
  {\lceil}}
\newcommand{\emitdeclend}{\ensuremath{\rceil}\lower .3ex
  \hbox to 0pt{\hss$\rceil$}}
\newcommand{\emitstmt}[1]{\emitstmtbegin#1\emitstmtend}
\newcommand{\emitdecl}[1]{\emitdeclbegin#1\emitdeclend}
\newcommand{\bqt}[1]{\underdots{#1}}
\newbox\underdotbox
\newcommand{\underdots}[1]{\ifmmode
  \setbox\underdotbox\hbox{$#1$}\else
  \setbox\underdotbox\hbox{#1}\fi\@tempdima=\dp\underdotbox
  \@tempdimb=\wd\underdotbox  
  \dp\underdotbox=0pt
  {\baselineskip=0pt\lineskip=0pt
    \setbox0\hbox{\vtop{\box\underdotbox\kern.6ex
        \hbox to \@tempdimb{\kern-.2ex\leaders
          \hbox{\footnotesize\kern-.2ex.}\hfill
          \kern-.2ex}}}\dp0=\@tempdima\box0}}

% Environments
\newenvironment{pseudocode}{\begin{tabbing}\quad\=\quad\=\quad\=\quad\=\quad\=\quad\=\quad\=\quad\=\quad\=\quad\=\kill}{\end{tabbing}}

%\theorembodyfont{\rmfamily\upshape}
\newtheorem{example}{Example}[section]
\newtheorem{Def}{Definition}[section]
\newtheorem{constraint}{Constraint}[section]

% The frameit environment formats text within a single 
% Anything can be framed, including verbatim text.

\def\doframeit#1{\vbox{%
  \hrule height\fboxrule
    \hbox{%
      \vrule width\fboxrule \kern\fboxsep
      \vbox{\kern\fboxsep #1\kern\fboxsep }%
      \kern\fboxsep \vrule width\fboxrule }%
    \hrule height\fboxrule }}

\def\frameit{\smallskip \advance \linewidth by -7.5pt \setbox0=\vbox \bgroup
\strut \ignorespaces }

\def\endframeit{\ifhmode \par \nointerlineskip \fi \egroup
\doframeit{\box0}}

% Xy-Pic section

% arrow heads for bending arrows
\makeatother
\newdir{/>}{:a(10)@{>}}
\makeatletter

% A box 1ex wide, 1ex high, 0ex deep with the contents centered
\def\exbox{\squarebox[1ex]}
\def\squarebox[#1]#2{\setbox0=\hbox to #1{\hss#2\hss}\dp0=0pt\ht0=#1\box0}

% \mathword{<prefix>}{<style>}{<identifier>} defines \<prefix><identifier>
%   to be a command usable in math mode which typesets <identifier> in the
%   given <style> (it,bf,sf,rm,tt,cal)
% \mathword{<prefix>}{<style>}[<any TeX input>]{<identifier>} defines
%   \<prefix><identifier> to be a command usable in math mode which
%   typesets <any TeX input> in the given style (it,bf,sf,rm,tt,cal)
\newcommand{\mathword}[2]{\@ifnextchar[{\@newsymbol{#1}{#2}}{\new@symbol{#1}{#2}}}
\newcommand{\new@symbol}[3]{\@newsymbol{#1}{#2}[#3]{#3}}
\def\@newsymbol#1#2[#3]#4{\expandafter\newcommand\csname
  #1#4\endcsname{\ensuremath{\mathop{\csname math#2\endcsname{#3}}\nolimits}}}

% Domains
\mathword{D}{cal}{B}
\mathword{D}{cal}{D}
\mathword{D}{cal}{E}
\mathword{D}{cal}{F}
\let\DJ\undefined
\mathword{D}{cal}{J}
\mathword{D}{cal}{P}
\mathword{D}{cal}{S}
\mathword{D}{cal}{T}
\mathword{D}{cal}{U}
\mathword{D}{it}{Aloc}
\mathword{D}{it}{pgenCode}
\mathword{D}{it}{ProgramPoint}
\mathword{D}{it}{ResidualLabel}
\mathword{D}{it}{Assignment}
\mathword{D}{it}{Names}
\mathword{D}{it}{Nil}
\mathword{D}{it}{bool}
\mathword{D}{it}{Label}
\mathword{D}{it}{List}
\mathword{D}{it}{void}
\mathword{D}{it}{Object}
\mathword{D}{it}{Pointer}
\mathword{D}{it}{Size}
\mathword{D}{it}{State}
\mathword{D}{it}{Store}
\mathword{D}{it}{String}
\mathword{D}{it}{Syntax}
\mathword{D}{it}{Type}

% Sets
\mathword{S}{cal}{C}
\mathword{S}{cal}[C_{\mathit{iu}}]{Ciu}
\mathword{S}{cal}[C_{\mathit{mw}}]{Cmw}

% Operators
\mathword{O}{cal}{L}
\mathword{O}{cal}{M}
\mathword{O}{cal}{PT}
\mathword{O}{cal}{U}
\mathword{O}{cal}{IU}
\mathword{O}{cal}{MW}
\mathword{O}{cal}{W}
\mathword{O}{cal}[W_B]{Wb}
\mathword{O}{cal}[W_F]{Wf}
\mathword{O}{cal}[W_J]{Wj}
\mathword{O}{cal}[W_S]{Ws}
\mathword{O}{cal}[K_B]{Kb}
\mathword{O}{cal}[K_F]{Kf}
\mathword{O}{cal}[K_J]{Kj}
\mathword{O}{cal}[K_P]{Kp}
\mathword{O}{cal}[K_S]{Ks}

% Types
\mathword{T}{it}{CallType}
\mathword{T}{it}{Code}
\mathword{T}{it}{DataObjectCopy}
\mathword{T}{it}{DataObject}
\mathword{T}{it}{FunObject}
\mathword{T}{it}{State}
\mathword{T}{it}{IUInfo}
\mathword{T}{it}{bool}
\mathword{T}{it}{char}
\mathword{T}{it}{int}
\mathword{T}{it}{struct}
\mathword{T}{it}{union}
\mathword{T}{it}{void}

% Constructors
\mathword{C}{sf}{Abstract}
\mathword{C}{sf}{Assign}
\mathword{C}{sf}{Array}
\mathword{C}{sf}{Binary}
\mathword{C}{sf}{Black}
\mathword{C}{sf}{Calloc}
\mathword{C}{sf}{Call}
\mathword{C}{sf}{Cast}
\mathword{C}{sf}{Const}
\mathword{C}{sf}{Unf}
\mathword{C}{sf}{Enum}
\mathword{C}{sf}{EnumMem}
\mathword{C}{sf}{Free}
\mathword{C}{sf}{Fun}
\mathword{C}{sf}{FunT}
\mathword{C}{sf}{Float}
\mathword{C}{sf}{Goto}
\mathword{C}{sf}{Gray}
\mathword{C}{sf}{If}
\mathword{C}{sf}{Int}
\mathword{C}{sf}{Lval}
\mathword{C}{sf}{Mem}
\mathword{C}{sf}{Malloc}
\mathword{C}{sf}{Member}
\mathword{C}{sf}{Nil}
\mathword{C}{sf}{NoConst}
\mathword{C}{sf}{NoConsts}
\mathword{C}{sf}{NoDecl}
\mathword{C}{sf}{NoName}
\mathword{C}{sf}{NoVar}
\mathword{C}{sf}{NoExp}
\mathword{C}{sf}{Null}
\mathword{C}{sf}{PAssign}
\mathword{C}{sf}{PCall}
\mathword{C}{sf}{PStrtAssign}
\mathword{C}{sf}{Pointer}
\mathword{C}{sf}{Return}
\mathword{C}{sf}{Rval}
\mathword{C}{sf}{Sequence}
\mathword{C}{sf}{StrtAssign}
\mathword{C}{sf}{StructMem}
\mathword{C}{sf}{Struct}
\mathword{C}{sf}{Unary}
\mathword{C}{sf}{Union}
\mathword{C}{sf}{User}
\mathword{C}{sf}{Var}
\mathword{C}{sf}{White}
\mathword{C}{sf}{false}
\mathword{C}{sf}{true}

% Variables
\mathword{V}{it}{DFSPred}
\mathword{V}{it}{SCCs}
\mathword{V}{it}{SCC}
\mathword{V}{it}{activeState}
\mathword{V}{it}{allNames}
\mathword{V}{it}{as}
\mathword{V}{it}{attempt}
\mathword{V}{it}{bop}
\mathword{V}{it}{callType}
\mathword{V}{it}{changed}
\mathword{V}{it}{cmp}
\mathword{V}{it}{color}
\mathword{V}{it}{copy}
\mathword{V}{it}{cpy}
\mathword{V}{it}{currentFun}
\mathword{V}{it}{cs}
\mathword{V}{it}{dataObject}
\mathword{V}{it}{dd}
\mathword{V}{it}{declEmitted}
\mathword{V}{it}{dep}
\mathword{V}{it}{dis}
\mathword{V}{it}{ds}
\mathword{V}{it}{dyn}
\mathword{V}{it}{emptyBody}
\mathword{V}{it}{endState}
\mathword{V}{it}{fcns}
\mathword{V}{it}{finishedFcnsInReverse}
\mathword{V}{it}{functions}
\mathword{V}{it}{fun}
\mathword{V}{it}{funStack}
\mathword{V}{it}{globals}
\mathword{V}{it}{id}
\mathword{V}{it}{inUse}
\mathword{V}{it}{ID}
\mathword{V}{it}{initEmitted}
\mathword{V}{it}{isRecursive}
\mathword{V}{it}{is}
\mathword{V}{it}{labels}
\mathword{V}{it}{link}
\mathword{V}{it}{locals}
\mathword{V}{it}{mem}
\mathword{V}{it}{ms}
\mathword{V}{it}{name}
\mathword{V}{it}{obj}
\mathword{V}{it}{ocs}
\mathword{V}{it}{oc}
\mathword{V}{it}{oe}
\mathword{V}{it}{oid}
\mathword{V}{it}{palloc}
\mathword{V}{it}{pendSet}
\mathword{V}{it}{pendSetStack}
\mathword{V}{it}{pgenLabel}
\mathword{V}{it}{pini}
\mathword{V}{it}{preStmt}
\mathword{V}{it}{preds}
\mathword{V}{it}{processedSet}
\mathword{V}{it}{psd}
\mathword{V}{it}{recentSet}
\mathword{V}{it}{recur}
\mathword{V}{it}{ref}
\mathword{V}{it}{resid}
\mathword{V}{it}{resLabel}
\mathword{V}{it}{retVal}
\mathword{V}{it}{rr}
\mathword{V}{it}[\textit{seenB4}]{seenBefore}
\mathword{V}{it}{sideEffects}
\mathword{V}{it}{sis}
\mathword{V}{it}{size}
\mathword{V}{it}{sp}
\mathword{V}{it}{ss}
\mathword{V}{it}{state}
\mathword{V}{it}{structs}
\mathword{V}{it}{symbolTable}
\mathword{V}{it}{symbolTableStack}
\mathword{V}{it}{ta}
\mathword{V}{it}{tb}
\mathword{V}{it}{temp}
\mathword{V}{it}{this}
\mathword{V}{it}{time}
\mathword{V}{it}{tmpRetVal}
\mathword{V}{it}{total}
\mathword{V}{it}{uop}
\mathword{V}{it}{us}
\mathword{V}{it}{ut}
\mathword{V}{it}{val}
\mathword{V}{it}{vars}
\mathword{V}{it}{workQ}
\mathword{V}{it}{zero}
\newcommand{\optdelta}{{o\delta}}
\newcommand{\deltas}{{\delta\mkern-2mu s}}

% Functions
\mathword{F}{rm}[splitArgs_{\mathcal{D}}]{splitArgsd}
\mathword{F}{rm}[splitArgs_{\mathcal{E}}]{splitArgse}
\mathword{F}{rm}[splitArgs_{\mathcal{T}}]{splitArgst}
\mathword{F}{rm}[splitArgs_{\mathcal{U}}]{splitArgsu}
%\mathword{F}{rm}{splitDecls}
\mathword{F}{rm}[split_{\DB}]{splitb}
\mathword{F}{rm}[split_{\DD}]{splitd}
\mathword{F}{rm}[split_{\DE}]{splite}
\mathword{F}{rm}[split_{\DJ}]{splitj}
\mathword{F}{rm}[split_{\DP}]{splitp}
\mathword{F}{rm}[split_{\DS}]{splits}
\mathword{F}{rm}[split_{\DT}]{splitt}
\mathword{F}{rm}[split_{\DU}]{splitu}
\mathword{F}{rm}{DFSVisitReverse}
\mathword{F}{rm}{DFSVisit}
\mathword{F}{rm}{ID}
\mathword{F}{rm}{ReadOnly}
\mathword{F}{rm}{add}
\mathword{F}{rm}{addEndState}
\mathword{F}{rm}{addRetVal}
\mathword{F}{rm}{assignRetVal}
\mathword{F}{rm}{beginBlock}
\mathword{F}{rm}{bottom}
\mathword{F}{rm}{bt}
\mathword{F}{rm}{calloc}
\mathword{F}{rm}{class}
\mathword{F}{rm}{cmpReferencedObject}
\mathword{F}{rm}{cmpWithOffset}
\mathword{F}{rm}{cmpSimple}
\mathword{F}{rm}{cmp}
\mathword{F}{rm}{concat}
\mathword{F}{rm}{cons}
\mathword{F}{rm}{copyReferencedObject}
\mathword{F}{rm}{copyWithOffset}
\mathword{F}{rm}{copySimple}
\mathword{F}{rm}{copy}
\mathword{F}{rm}{dyn}
\mathword{F}{rm}{emitZeroDecls}
\mathword{F}{rm}{emitZeroInit}
\mathword{F}{rm}{emit}
\mathword{F}{rm}{emptyQ}
\mathword{F}{rm}{emptyset}
\mathword{F}{rm}{empty}
\mathword{F}{rm}{endBlock}
\mathword{F}{rm}{equal}
\mathword{F}{rm}{find}
\mathword{F}{rm}{free}
\mathword{F}{rm}{freshGlobalName}
\mathword{F}{rm}{freshLocalName}
\mathword{F}{rm}{freshResidFun}
\mathword{F}{rm}{freshResidLabel}
\mathword{F}{rm}{freshResidVar}
\mathword{F}{rm}{freshVar}
\mathword{F}{rm}{fresh}
\mathword{F}{rm}[functionSeenB4]{functionSeenBefore}
\mathword{F}{rm}{generate}
\mathword{F}{rm}{get}
\mathword{F}{rm}{hd}
\mathword{F}{rm}{initZeros}
\mathword{F}{rm}{initialize}
\mathword{F}{rm}{isInUse}
\mathword{F}{rm}{isRecursive}
\mathword{F}{rm}{liftFP}
\mathword{F}{rm}{lift}
\mathword{F}{rm}{link}
\mathword{F}{rm}{localCopy}
\mathword{F}{rm}{localEqual}
\mathword{F}{rm}{lookupbb}
\mathword{F}{rm}{lookupfun}
\mathword{F}{rm}{lookupvar}
\mathword{F}{rm}{lookup}
\mathword{F}{rm}{makeSCCs}
\mathword{F}{rm}{malloc}
\mathword{F}{rm}{memcmp}
\mathword{F}{rm}{memcpy}
\mathword{F}{rm}{newtemp}
\mathword{F}{rm}{new}
\mathword{F}{rm}{nonLocalCopy}
\mathword{F}{rm}{not}
\mathword{F}{rm}{object}
\mathword{F}{rm}{outputResidualProgram}
\mathword{F}{rm}{pendInsert}
\mathword{F}{rm}{pending}
\mathword{F}{rm}{pendingSP}
\mathword{F}{rm}{popFun}
\mathword{F}{rm}{popObjects}
\mathword{F}{rm}{popPendSet}
\mathword{F}{rm}{popSP}
\mathword{F}{rm}{preds}
\mathword{F}{rm}{pred}
\mathword{F}{rm}{psdPtr}
\mathword{F}{rm}{pushFun}
\mathword{F}{rm}{pushPendSet}
\mathword{F}{rm}{push}
\mathword{F}{rm}{remove}
\mathword{F}{rm}{removeMinTime}
\mathword{F}{rm}{restore}
\mathword{F}{rm}{setRetVal}
\mathword{F}{rm}{sharable}
\mathword{F}{rm}{sizeof}
\mathword{F}{rm}{size}
\mathword{F}{rm}{splitArgs}
\mathword{F}{rm}{splitInits}
\mathword{F}{rm}{splitMems}
\mathword{F}{rm}{splitable}
\mathword{F}{rm}{split}
\mathword{F}{rm}{stat}
\mathword{F}{rm}{succs}
\mathword{F}{rm}{time}
\mathword{F}{rm}{tl}
\mathword{F}{rm}{top}
\mathword{F}{rm}{toStr}
\mathword{F}{rm}{unionTypes}
\mathword{F}{rm}{union}
\mathword{F}{rm}{update}
\mathword{F}{rm}{value}
\mathword{F}{rm}{visited}
\mathword{F}{rm}{zero}

% Keywords
\mathword{K}{bf}{all}
\mathword{K}{bf}{case}
\mathword{K}{bf}{do}
\mathword{K}{bf}{else}
\mathword{K}{bf}{error}
\mathword{K}{bf}{extern}
\mathword{K}{bf}{for}
\mathword{K}{bf}{goto}
\mathword{K}{bf}{if}
\mathword{K}{bf}[\#include]{include}
\mathword{K}{bf}{let}
\mathword{K}{bf}{in}
\mathword{K}{bf}{not}
\mathword{K}{bf}{of}
\mathword{K}{bf}{return}
\mathword{K}{bf}{then}
\mathword{K}{bf}{to}
\mathword{K}{bf}{var}
\mathword{K}{bf}{where}
\mathword{K}{bf}{while}

% Labels
\mathword{L}{it}{pendLoop}
%\mathword{L}{it}{firstLabel}
\mathword{L}{it}{basicBlock}
\mathword{L}{it}{funcEnd}

% Programs
\mathword{P}{it}[p_{ann}]{pann}
\mathword{P}{it}[p_{gen}]{pgen}
\mathword{P}{it}[p_{res}]{pres}
\mathword{P}{it}[p_{split}]{psplit}

% Shorthands for symbols
\newcommand{\powset}{\mathcal{P}}

% Math ligatures
\mathlig{|->}{\mapsto}
\mathlig{<-}{\leftarrow}
\mathlig{<--}{\mathop{\longleftarrow}\limits}
\mathlig{->}{\rightarrow}
\mathlig{-->}{\mathop{\longrightarrow}\limits}
\mathlig{--->}{\mathop{\relbar\joinrel\relbar\joinrel\rightarrow}\limits}
\mathlig{---->}{\mathop{\relbar\joinrel\relbar\joinrel\relbar\joinrel
    \relbar\joinrel\rightarrow}\limits}
\mathlig{=>}{\Rightarrow}
\mathlig{==>}{\Longrightarrow}
\mathlig{===>}{\mathop{=\joinrel=\joinrel\Rightarrow}\limits}
\mathlig{<=}{\Leftarrow}
\mathlig{<==}{\Longleftarrow}
\mathlig{<=>}{\Leftrightarrow}
\mathlig{<==>}{\iff}
\mathlig{[[}{\mathopen{[\mskip-2.5mu[}}
\mathlig{]]}{\mathclose{]\mskip-2.5mu]}}
\mathlig{]]]}{\mathclose{]\mskip-2.5mu]\mskip-2.5mu]}}
\mathlig{[]}{\mathord{[\mskip 3mu]}}
\mathlig{==}{\equiv}
\mathlig{=<}{\le}
\mathlig{>=}{\ge}
\mathlig{!=}{\mathrel{!\!=}}
\mathlig{++}{\mathbin{++}}
\mathlig{...}{\ldots}

\mathlig{+>}{\unrhd}
\mathlig{++>}{\blacktriangleright}
\mathlig{>>=}{\succeq}

\newcommand{\coreXref}[1]{\langle\!|#1|\!\rangle}

\AtBeginDocument{\mathligson}

\pagestyle{myheadings}
\@tempcnta=\the\time\divide\@tempcnta by 60\relax
\ifnum\@tempcnta<10\def\timenow{0}\fi
\edef\timenow{\the\@tempcnta:}
\multiply\@tempcnta by 60\relax\@tempcntb=-\@tempcnta
\advance\@tempcntb by\time
\ifnum\@tempcntb<10\edef\timenow{\timenow0}\fi
\edef\timenow{\timenow\the\@tempcntb}
\markboth{\hfill\today\hfill\llap{\cmix\ documentation}}
{\hfill\today\hfill}

\renewcommand{\sectionmark}[1]{\markboth
{\hfill\today\hfill\llap{\cmix\ documentation}}
{\rlap{\thesection. #1}\hfill\today\hfill}}

%%% Local Variables: 
%%% mode: latex
%%% TeX-master: "cmixII"
%%% End: 

