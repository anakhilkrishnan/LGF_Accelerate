import textwrap, os, sys

# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'LGF_Accelerate'
copyright = '2026, Akhil Krishnan'
author = 'Akhil Krishnan'
release = '1.0.0'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = ["breathe", "exhale", "sphinxcontrib.bibtex"]

templates_path = ['_templates']
exclude_patterns = []

# -- Options for bibliography ------------------------------------------------
bibtex_bibfiles = ["manual/refs.bib"]
bibtex_default_style = "plain"

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

# -- Options for MathJax output -------------------------------------------

mathjax3_config = {
    'tex': {
        'macros': {
            'odif': [r'\mathrm{d}#1', 1],        # usage: \odif{x} -> dx
            'abs': [r'\left|#1\right|', 1],     # usage: \abs{x} -> |x|
        }
    }
}

html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']

# -- Breathe configuration ---------------------------------------------------

breathe_projects        = {"LGF_accelerate": "./_doxygen/xml"}
breathe_default_project = "LGF_accelerate"

# -- Exhale configuration ----------------------------------------------------
# exhale_args drives Doxygen AND generates the rst file tree automatically
exhale_args = {
    # where Exhale writes the generated .rst API files
    "containmentFolder":    "./api",
    "rootFileName":         "library_root.rst",
    "rootFileTitle":        "LGF Core API",
    "doxygenStripFromPath": "../../Src",

    # Doxygen input: path(s) relative to docs/
    "exhaleExecutesDoxygen": True,
    "exhaleDoxygenStdin": textwrap.dedent("""
    INPUT                = ../../Src
    FILE_PATTERNS        = *.H *.hpp
    RECURSIVE            = YES
    EXTRACT_ALL          = YES
    GENERATE_XML         = YES
    GENERATE_HTML        = NO
    GENERATE_LATEX       = NO
    QUIET                = YES
    WARN_IF_UNDOCUMENTED = NO
    EXCLUDE_PATTERNS     = */main.cpp \
                           */ThirdParty/BBFMM2D/*
    ENABLE_PREPROCESSING = YES
    MACRO_EXPANSION      = YES
    PREDEFINED          += AMREX_GPU_DEVICE= \\
                           AMREX_GPU_HOST_DEVICE= \\
                           AMREX_FORCE_INLINE=inline \\
                           AMREX_GPU_DEVICE_MANAGED=
    """),
}

# Note: When making changes to the above before rebuilding, always perform
# (from the Docs/ dir)
# rm -rf source/_doxygen source/api source/_build source/.doctrees
# Additionally, cleanup pre-existing builds
# rm -rf build
# to ensure that any stored data is wiped before fresh build

# Note: To produce a fresh build, perform
# sphinx-build -b html source build/html
# sphinx-build -b latex source build/latex
# run pdflatex + latexmk internally
# cd build/latex && make   

# -- LaTeX Configuration -----------------------------------------------------

latex_elements = {
    # Paper size
    "papersize": "a4paper",

    # Font size
    "pointsize": "11pt",

    # Preamble additions (e.g. extra packages)
    "preamble": r"""
        \usepackage{lmodern}
        \usepackage{microtype}
        \usepackage{amsmath}
        \usepackage{bm}
        \usepackage{physics}
        \usepackage{amssymb}
        \usepackage{derivative}
        \usepackage{float}
    """,

    # Put the table of contents on its own page
    "tableofcontents": r"\tableofcontents\newpage",
}

# Controls the PDF document structure
latex_documents = [
    (
        "index",              # source start file (index.rst)
        "LGF_core.tex",       # output .tex filename
        "LGF Accelerate Docs",# document title
        "Akhil Krishnan",          # author
        "manual",             # documentclass: 'manual' or 'howto'
    ),
]