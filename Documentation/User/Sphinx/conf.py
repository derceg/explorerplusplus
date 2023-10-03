# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'Explorer++'
copyright = '2023, Explorer++'
author = 'Allen Titley'
version = '1.4.0'
release = '1.4.0'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'sphinx.ext.autosectionlabel',
]

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store', 'global.rst']

# global.rst contains some global declarations that may be used across the
# project.
rst_prolog = """
.. include:: /global.rst
"""

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'
html_theme_options = {
    'collapse_navigation': False,
    'titles_only': True
}

html_static_path = ['_static']

def setup(app):
    app.add_css_file('css/custom.css')

# -- Extension configuration -------------------------------------------------

# -- Options for autosectionlabel extension ----------------------------------

autosectionlabel_prefix_document = True
