# InterText Editor

InterText is an editor for aligned parallel texts. It has been developed for the project [InterCorp](http://www.korpus.cz/intercorp/?lang=en) to edit and manage alignments of multiple parallel language versions of texts at the level of sentences, but it is designed with flexibility in mind and supports custom XML documents and Unicode character set.

_InterText Editor_ is a younger brother of the _InterText Server_, a server application with web-based interface aimed at large, collaborative projects. InterText Editor is a desktop application for personal use, but it can also be used as an off-line editor for InterText Servers. Unlike InterText Server, it should be very easy to install and use even for common users.

See the [InterText project homepage](http://wanthalf.saga.cz/intertext) for more details.

## Compilation

InterText Editor is implemented using the current version of [Qt toolkit](http://www.qt.io/) to ease platform independency. The project can be easilly compiled using the standard _QtCreator_ development environment or the _qmake_ tool from the Qt distribution. The compilation from the source code has no special dependencies beyond the standard Qt toolkit libraries (and possibly (Open)SSL library in order to support HTTPS communication with InterText Server, if required; SSL support for Qt is commonly available in Linux and MacOS X, but may require additional installation or compilation on Microsoft Windows). 

## Precompiled binary packages

See the [InterText project homepage](http://wanthalf.saga.cz/intertext) for binary distribution packages precompiled for Linux, MacOS X and Microsoft Windows.

## Disclaimer

The code of InterText Editor is not a pruduct of an organized professional development team. Actually, it is the first larger C++ project of the author and bears clear traces of "patchwork". It may also contain fragments of unfinished new features. Please, feel free to contact the author in order to get help with quicker orientation in the code in case of any uncertainity. Any submissions, improvements or suggestions are welcome!

## Acknowledgement:

This software and documentation resulted from the implementation of the Czech National Corpus project (LM2011023) funded by the _Ministry of Education, Youth and Sports_ of the Czech republic within the framework of _Large Research, Development and Innovation Infrastructures_.

## License:

This software is licensed under the GNU General Public License v3. (http://www.gnu.org/licenses/gpl-3.0.html)

- Copyright (c) 2010-2016 Pavel Vondřička
- Copyright (c) 2010-2016 Charles University in Prague, Faculty of Arts, Institute of the Czech National Corpus
