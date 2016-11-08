# $Id$
# Check for Qt compiler flags, linker flags, and binary packages
#
# Copyright (C) 2002-2014 by Thomas Dreibholz
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Contact: dreibh@iem.uni-due.de

AC_DEFUN([TD_CHECK_QT],
[

QT_REQUIRED_COMPONENTS="QtWidgets QtGui QtXml QtCore"
QT_DEFAULT_INCLUDE_PATHS="/usr/share/qt5/include /usr/local/include/qt5 `find /usr/include -name qt5 -type d | head -n1`"
QT_DEFAULT_LIBRARY_PATHS="/usr/lib /usr/local/lib /usr/local/qt5/lib /usr/local/lib/qt5 `cat 2>/dev/null /etc/ld.so.conf.d/*.conf | sed -e "/# /d"` /usr/lib64"
QT_DEFAULT_BINARY_PATHS="/usr/bin /usr/local/bin /usr/local/qt5/bin /usr/share/qt5/bin"

QTEXTRAINC=""
QTEXTRALIB=""
QT_LDADD=""
QT_CXXFLAGS=""
MOC=""


# ====== Directory ==========================================================
AC_ARG_WITH([qt-dir],
   AC_HELP_STRING([--with-qt-dir=/path/to/qt],
      [to specify the path to the Qt directory.]),
   [QTDIR="$withval"],
   [QTDIR=""])
if test x$QTDIR != x ; then
   AC_MSG_CHECKING(Qt directory)
   if test ! -d $x ; then
      AC_MSG_ERROR([Bad setting of Qt directory. Check --qt-dir parameter!])
   fi
   AC_MSG_RESULT([okay])
fi


# ====== Includes ===========================================================
AC_MSG_CHECKING(Qt includes directory)
AC_ARG_WITH([qt-include],
   AC_HELP_STRING([--with-qt-include=/path/to/qt include],
      [to specify the path to the Qt include directory]),
   [QT_INCLUDE_PATHS="$withval"],
   [QT_INCLUDE_PATHS="$QT_DEFAULT_INCLUDE_PATHS"])
if test x$QTDIR != x ; then
   QT_INCLUDE_PATHS="$QTDIR/include"
fi

QTEXTRAINC=""
for x in $QT_INCLUDE_PATHS ; do
    echo -n "$x?   "
    if test -d $x ; then
       QTEXTRAINC="$x"
       break
    fi
done
if test x$QTEXTRAINC = x ; then
   AC_MSG_ERROR([No Qt include directory found. Try --with-qt-include=<directory>.])
fi
AC_MSG_RESULT([-->   $QTEXTRAINC])


# ====== Libraries ==========================================================
AC_MSG_CHECKING(Qt libraries directory)
AC_ARG_WITH([qt-lib],
   AC_HELP_STRING([--with-qt-lib=/path/to/qt lib],
      [to specify the path to the Qt lib directory]),
   [QT_LIBRARY_PATHS="$withval"],
   [QT_LIBRARY_PATHS="$QT_DEFAULT_LIBRARY_PATHS"])
if test x$QTDIR != x ; then
   QT_LIBRARY_PATHS="$QTDIR/lib"
fi

QTEXTRALIB=""
for x in $QT_LIBRARY_PATHS ; do
    echo -n "$x?   "
    if test -d $x ; then
       # The libraries directory must contain libQtCore. Otherwise,
       # this directory is not the right one!
       if test -f $x/libQt5Core.so -o -f $x/libQt5Core.a ; then
          QTEXTRALIB="$x"
          break
       fi
    fi
done
if test x$QTEXTRALIB = x ; then
   AC_MSG_ERROR([No Qt libraries directory found. Try --with-qt-lib=<directory>.])
fi
AC_MSG_RESULT([-->   $QTEXTRALIB])


# ====== Components =========================================================
QT_CXXFLAGS="$QT_CXXFLAGS -fPIC -I$QTEXTRAINC"
QT_LDADD="-L$QTEXTRALIB"   # OLD: "-Wl,-rpath,$QTEXTRALIB -L$QTEXTRALIB"
for x in $QT_REQUIRED_COMPONENTS ; do
   AC_MSG_CHECKING([Qt component $x])
   x5=`echo "$x" | sed -e "s/Qt/Qt5/g"`
   if ! test -e $QTEXTRALIB/lib$x5.so ; then
      if ! test -e $QTEXTRALIB/lib$x5.a ; then
         AC_MSG_ERROR([Component $x is not available: no $QTEXTRALIB/lib$x5.so or $QTEXTRALIB/lib$x5.a found!])
      fi
   fi
   AC_MSG_RESULT([okay])
   QT_CXXFLAGS="$QT_CXXFLAGS -I$QTEXTRAINC/$x"
   QT_LDADD="$QT_LDADD -l$x5"
done
QT_LDADD="$QT_LDADD $X_PRE_LIBS $X_LIBS $X_EXTRA_LIBS -lpthread"

AC_MSG_CHECKING([QT_CXXFLAGS])
AC_MSG_RESULT([$QT_CXXFLAGS])
AC_MSG_CHECKING([QT_LDADD])
AC_MSG_RESULT([$QT_LDADD])

AC_SUBST(QT_CXXFLAGS)
AC_SUBST(QT_LDADD)


# ====== Programs ===========================================================
AC_ARG_WITH([qt-bin],
   AC_HELP_STRING([--with-qt-binaries=/path/to/qt binaries],
      [to specify the path to the Qt binaries directory]),
   [QT_BINARY_PATHS="$withval"],
   [QT_BINARY_PATHS="$QT_DEFAULT_BINARY_PATHS"])
if test x$QTDIR != x ; then
   QT_BINARY_PATHS="$QTDIR/bin"
fi

AC_MSG_CHECKING(Looking for qtchooser)
QTCHOOSER="qtchooser"
for p in $QT_BINARY_PATHS $QT_LIBRARY_PATHS ; do
   if test -x $p/qtchooser/$QTCHOOSER ; then
      QTCHOOSER="$p/qtchooser/$QTCHOOSER"
      break
   elif test -x $p/$QTCHOOSER ; then
      QTCHOOSER="$p/$QTCHOOSER"
      break
   fi
done
if ! test -x $QTCHOOSER ; then
   AC_ERROR([qtchooser not found!])
fi
AC_MSG_RESULT([$QTCHOOSER])
AC_SUBST(QTCHOOSER)

AC_MSG_CHECKING(Qt meta-object compiler moc)
MOC="$QTCHOOSER -run-tool=moc -qt=5"
AC_MSG_RESULT([$MOC])
AC_SUBST(MOC)


# ====== Tests ==============================================================
cat > myqt.h << EOF
   #include <QObject>

   class Test : public QObject
   {
      Q_OBJECT

      public:
         Test() {}
         ~Test() {}

      public slots:
         void receive() {}

      signals:
         void send();
   };
EOF

cat > myqt.cpp << EOF
   #include "myqt.h"
   #include <QtWidgets/QApplication>

   int main( int argc, char **argv )
   {
      QApplication app( argc, argv );
      Test t;
      QObject::connect( &t, SIGNAL(send()), &t, SLOT(receive()) );
   }
EOF


# ------ MOC test -----------------------------------------------------------
AC_MSG_CHECKING(for working $MOC)
test_command_1="$MOC myqt.h -o moc_myqt.cpp"
AC_TRY_EVAL(test_command_1)
if test x"$ac_status" != x0; then
   AC_MSG_ERROR($MOC call failed)
fi
AC_MSG_RESULT(yes)

# ------ MOC compile test ---------------------------------------------------
AC_MSG_CHECKING(whether C++-compiling the moc-generated program works)
test_command_2="$CXX $QT_CXXFLAGS -c $CXXFLAGS -o moc_myqt.o moc_myqt.cpp"

AC_TRY_EVAL(test_command_2)
if test x"$ac_status" != x0; then
   AC_MSG_ERROR($MOC-generated test program C++-compilation failed)
fi
AC_MSG_RESULT(yes)

# ------ Compile test -------------------------------------------------------
AC_MSG_CHECKING(whether C++-compiling a test program works)
test_command_3="$CXX $QT_CXXFLAGS -c $CXXFLAGS -o myqt.o myqt.cpp"
AC_TRY_EVAL(test_command_3)
if test x"$ac_status" != x0; then
   AC_MSG_ERROR(test program C++-compilation failed)
fi
AC_MSG_RESULT(yes)

# ------ Link test ----------------------------------------------------------
AC_MSG_CHECKING(whether linking works)
nv_try_4="$CXX $QT_LDADD -o myqt myqt.o moc_myqt.o"
AC_TRY_EVAL(test_command_4)
if test x"$ac_status" != x0; then
   AC_MSG_ERROR(linking failed)
fi
AC_MSG_RESULT(yes)

# ------ Clean up -----------------------------------------------------------
rm -f moc_myqt.cpp myqt.h myqt.cpp myqt.o myqt moc_myqt.o

])
