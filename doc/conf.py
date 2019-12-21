# -*- coding: utf-8 -*-
#
# Project ACRN documentation build configuration file, created by
# sphinx-quickstart on Wed Jan 10 20:51:29 2018.
#
# This file is execfile()d with the current directory set to its
# containing dir.
#
# Note that not all possible configuration values are present in this
# autogenerated file.
#
# All configuration values have a default; values that are commented out
# serve to show the default.

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
import os
import sys
sys.path.insert(0, os.path.abspath('.'))

RELEASE = ""
if "RELEASE" in os.environ:
   RELEASE = os.environ["RELEASE"]

# -- General configuration ------------------------------------------------

# If your documentation needs a minimal Sphinx version, state it here.
#
# needs_sphinx = '1.0'

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.

sys.path.insert(0, os.path.join(os.path.abspath('.'), 'extensions'))
extensions = ['breathe', 'sphinx.ext.graphviz', 'sphinx.ext.extlinks',
              'kerneldoc', 'eager_only', 'html_redirects']

# extlinks provides a macro template

extlinks = {'acrn-commit': ('https://github.com/projectacrn/acrn-hypervisor/commit/%s', ''),
            'acrn-issue': ('https://github.com/projectacrn/acrn-hypervisor/issues/%s', '')
           }

# kernel-doc extension configuration for running Sphinx directly (e.g. by Read
# the Docs). In a normal build, these are supplied from the Makefile via command
# line arguments.

kerneldoc_bin = 'scripts/kernel-doc'
kerneldoc_srctree = '../../acrn-kernel'


graphviz_output_format='png'
graphviz_dot_args=[
   '-Nfontname="verdana"',
   '-Gfontname="verdana"',
   '-Efontname="verdana"']

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# The suffix(es) of source filenames.
# You can specify multiple suffix as a list of string:
#
# source_suffix = ['.rst', '.md']
source_suffix = '.rst'

# The master toctree document.
master_doc = 'index'

# General information about the project.
project = u'Project ACRN™'
copyright = u'2019, Project ACRN'
author = u'Project ARCN developers'

# The version info for the project you're documenting, acts as replacement for
# |version| and |release|, also used in various other places throughout the
# built documents.

# The following code tries to extract the information by reading the
# Makefile from the acrn-hypervisor repo by finding these lines:
#   MAJOR_VERSION=1
#   MINOR_VERSION=3
#   EXTRA_VERSION=-unstable

try:
    version_major = None
    version_minor = None
    version_rc = None

    for line in open(os.path.realpath("../../../VERSION")) :
        # remove comments
        line = line.split('#', 1)[0]
        line = line.rstrip()
        if (line.count("=") == 1) :
           key, val = [x.strip() for x in line.split('=', 2)]
           if key == 'MAJOR_VERSION':
              version_major = val
           if key == 'MINOR_VERSION':
              version_minor = val
           if key == 'EXTRA_VERSION':
              version_rc = val
           if version_major and version_minor and version_rc :
              break
finally:
    if version_major and version_minor :
        version = release = "v " + str(version_major) + '.' + str(version_minor)
        if version_rc :
          version = release = version + version_rc
    else:
        sys.stderr.write('Warning: Could not extract hypervisor version from VERSION file\n')
        version = release = "unknown"

#
# The short X.Y version.
# version = u'0.1'
# The full version, including alpha/beta/rc tags.
# release = u'0.1'

# The language for content autogenerated by Sphinx. Refer to documentation
# for a list of supported languages.
#
# This is also used if you do content translation via gettext catalogs.
# Usually you set "language" from the command line for these cases.
language = None

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This patterns also effect to html_static_path and html_extra_path
exclude_patterns = ['_build', 'misc/README.rst' ]

# The name of the Pygments (syntax highlighting) style to use.
pygments_style = 'sphinx'

# If true, `todo` and `todoList` produce output, else they produce nothing.
todo_include_todos = False

# -- Options for HTML output ----------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
try:
    import sphinx_rtd_theme
except ImportError:
    html_theme = 'alabaster'
    # This is required for the alabaster theme
    # refs: http://alabaster.readthedocs.io/en/latest/installation.html#sidebars
    html_sidebars = {
        '**': [
            'relations.html',  # needs 'show_related': True theme option to display
            'searchbox.html',
            ]
        }
    sys.stderr.write('Warning: sphinx_rtd_theme missing. Use pip to install it.\n')
else:
    html_theme = "sphinx_rtd_theme"
    html_theme_path = [sphinx_rtd_theme.get_html_theme_path()]
    html_theme_options = {
        'canonical_url': '',
        'analytics_id': '',
        'logo_only': False,
        'display_version': True,
        'prev_next_buttons_location': 'None',
        # Toc options
        'collapse_navigation': False,
        'sticky_navigation': True,
        'navigation_depth': 4,
    }


# Here's where we (manually) list the document versions maintained on
# the published doc website.  On a daily basis we publish to the
# /latest folder but when releases are made, we publish to a /<relnum>
# folder (specified via RELEASE=name on the make command).

if tags.has('release'):
   current_version = version
   if RELEASE:
      version = current_version = RELEASE
else:
   version = current_version = "latest"

html_context = {
   'current_version': current_version,
   'versions': ( ("latest", "/latest/"),
                 ("1.5", "/1.5/"),
                 ("1.4", "/1.4/"),
                 ("1.3", "/1.3/"),
                 ("1.2", "/1.2/"),
                 ("1.1", "/1.1/"),
                 ("1.0", "/1.0/"),   # keep 1.0
               )
    }


# Theme options are theme-specific and customize the look and feel of a theme
# further.  For a list of options available for each theme, see the
# documentation.
#
# html_theme_options = {}

html_logo = 'images/ACRN_Logo_200w.png'
html_favicon = 'images/ACRN-favicon-32x32.png'

numfig = True
#numfig_secnum_depth = (2)
numfig_format = {'figure': 'Figure %s', 'table': 'Table %s', 'code-block': 'Code Block %s'}

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['static']

def setup(app):
   app.add_stylesheet("acrn-custom.css")
   app.add_javascript("acrn-custom.js")

# Custom sidebar templates, must be a dictionary that maps document names
# to template names.
#

# If true, "Created using Sphinx" is shown in the HTML footer. Default is True.
html_show_sphinx = False

# If true, links to the reST sources are added to the pages.
html_show_sourcelink = False

# If not '', a 'Last updated on:' timestamp is inserted at every page
# bottom,
# using the given strftime format.
html_last_updated_fmt = '%b %d, %Y'

# -- Options for HTMLHelp output ------------------------------------------

# Output file base name for HTML help builder.
htmlhelp_basename = 'ACRN Project Help'


# -- Options for LaTeX output ---------------------------------------------

latex_elements = {
    # The paper size ('letterpaper' or 'a4paper').
    #
    # 'papersize': 'letterpaper',

    # The font size ('10pt', '11pt' or '12pt').
    #
    # 'pointsize': '10pt',

    # Additional stuff for the LaTeX preamble.
    #
    # 'preamble': '',

    # Latex figure (float) alignment
    #
    # 'figure_align': 'htbp',
}

# Grouping the document tree into LaTeX files. List of tuples
# (source start file, target name, title,
#  author, documentclass [howto, manual, or own class]).
latex_documents = [
    (master_doc, 'acrn.tex', u'Project ACRN Documentation',
     u'Project ACRN', 'manual'),
]


# -- Options for manual page output ---------------------------------------

# One entry per manual page. List of tuples
# (source start file, name, description, authors, manual section).
man_pages = [
    (master_doc, 'acrn', u'Project ACRN Documentation',
     [author], 1)
]


# -- Options for Texinfo output -------------------------------------------

# Grouping the document tree into Texinfo files. List of tuples
# (source start file, target name, title, author,
#  dir menu entry, description, category)
texinfo_documents = [
    (master_doc, 'Project ACRN', u'Project ACRN Documentation',
     author, 'Project ACRN', 
     'IoT-Optimized Hypervisor for Intel Architecture',
     'Miscellaneous'),
]

rst_epilog = """
.. include:: /substitutions.txt
"""


breathe_projects = {
	"Project ACRN" : "doxygen/xml",
}
breathe_default_project = "Project ACRN"
breathe_default_members = ('members', 'undoc-members', 'content-only')


# Custom added feature to allow redirecting old URLs (caused by
# reorganizing doc directories)
#
# list of tuples (old_url, new_url) for pages to redirect
#
# URLs must be relative to document root (with NO leading slash),
# and without the html extension)
html_redirect_pages = [
   ('developer-guides/index', 'contribute'),
   ('getting-started/index', 'try'),
   ('user-guides/index', 'develop'),
   ('hardware', 'reference/hardware'),
   ('release_notes', 'release_notes/index')
   ]
