# Check for Qt compiler flags, linker flags, and binary packages
AC_DEFUN([gw_CHECK_QT],
[
QTDIR=xxx

QTPOSTFIX=""
QTEXTRAINC="/usr/include/qt4"
QTEXTRALIB="/usr/share/qt4"

AC_ARG_WITH([qt-dir],
   AC_HELP_STRING([--with-qt-dir=/path/to/Qt-4.x.0],
      [to specify the path to the Qt-4.x.0 directory.]),
   [QTPATHS="$withval"],
   [QTPATHS="/usr/local/qt4 /usr/share/qt4"])

AC_ARG_WITH([qt-include],
   AC_HELP_STRING([--with-qt-include=/path/to/Qt-4.x include],
      [to specify the path to the Qt-4. include directory]),
   [QTEXTRAINC="$withval"],
   [QTEXTRAINC="/usr/include/qt4"])

for x in $QTPATHS; do
    if test -d $x ; then
       QTDIR="$x"
    fi
done


AC_MSG_CHECKING(Qt4.x directory)

if test $QTDIR = xxx ; then
   AC_MSG_ERROR(Could not locate QT 4.x)
fi

AC_MSG_RESULT($QTDIR)
AC_MSG_CHECKING(Qt4.x lib)

AC_ARG_WITH([qt-lib],
   AC_HELP_STRING([--with-qt-lib=/path/to/Qt-4.x lib],
      [to specify the path to the Qt-4. lib directory]),
   [QTEXTRALIB="$withval"],
   [QTEXTRALIB="$QTDIR/lib"])

AC_MSG_RESULT($QTEXTRALIB)


QT_LDADD="-Wl,-rpath,$QTEXTRALIB -L$QTEXTRALIB -lQtGui -lQtXml -lQtCore $X_LIBS -lX11 -lXext -lXmu -lXt -lXi $X_EXTRA_LIBS  -lpthread"
QT_CXXFLAGS="-I$QTEXTRAINC -I$QTEXTRAINC/QtGui -I$QTEXTRAINC/QtXml -I$QTEXTRAINC/QtCore -I$QTEXTRAINC/QtOpenGL $X_CFLAGS -DQT_OPENGL_LIB -DQT_GUI_LIB -DQT_CORE_LIB -DQT_SHARED -I$QTEXTRAINC"
QTBIN="$QTDIR/bin"


AC_MSG_CHECKING(Qt4.x includes)
AC_MSG_RESULT($QT_CXXFLAGS $QTEXTRAINC)

ADM_QT_LDADD="$QT_LDADD"
ADM_QT_CXXFLAGSLUDES="$QT_CXXFLAGS"
ADM_QTCFLAGS="$QT_CXXFLAGS"

MOC="$QTBIN/moc$QTPOSTFIX"
UIC="$QTBIN/uic$QTPOSTFIX"
RCC="$QTBIN/rcc"
AC_SUBST(MOC)
AC_SUBST(UIC)
AC_SUBST(RCC)

# Now we check whether we can actually build a Qt app.
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
   #include <QApplication>

   int main( int argc, char **argv )
   {
      QApplication app( argc, argv );
      Test t;
      QObject::connect( &t, SIGNAL(send()), &t, SLOT(receive()) );
   }
EOF


AC_MSG_CHECKING(for working moc)
test_command_1="$MOC myqt.h -o moc_myqt.cpp"
AC_TRY_EVAL(test_command_1)

if test x"$ac_status" != x0; then
   AC_MSG_ERROR(moc call failed)
fi

AC_MSG_RESULT(yes)
AC_MSG_CHECKING(whether C++-compiling the moc-generated program works)
test_command_2="$CXX $QT_CXXFLAGS -c $CXXFLAGS -o moc_myqt.o moc_myqt.cpp"
AC_TRY_EVAL(test_command_2)

if test x"$ac_status" != x0; then
   AC_MSG_ERROR(moc-generated test program C++-compilation failed)
fi


AC_MSG_RESULT(yes)
AC_MSG_CHECKING(whether C++-compiling a test program works)
test_command_3="$CXX $QT_CXXFLAGS -c $CXXFLAGS -o myqt.o myqt.cpp"
AC_TRY_EVAL(test_command_3)

if test x"$ac_status" != x0; then
   AC_MSG_ERROR(test program C++-compilation failed)
fi


AC_MSG_RESULT(yes)
AC_MSG_CHECKING(whether linking works)
nv_try_4="$CXX $LIBS $ADM_QT_LDADD -o myqt myqt.o moc_myqt.o"
AC_TRY_EVAL(test_command_4)

if test x"$ac_status" != x0; then
   AC_MSG_ERROR(linking failed)
fi


AC_MSG_RESULT(yes)
AC_MSG_CHECKING(for mkoctfile)
AC_TRY_EVAL(mkoctfile)

if test x"$ac_status" != x0; then
   AC_MSG_ERROR(mkoctfile is not in the path)
fi

AC_MSG_RESULT(yes)
rm -f moc_myqt.cpp myqt.h myqt.cpp myqt.o myqt moc_myqt.o
])


AC_MSG_CHECKING([QT_CXXFLAGS])
AC_MSG_RESULT([$QT_CXXFLAGS])
AC_MSG_CHECKING([QT_LDADD])
AC_MSG_RESULT([$QT_LDADD])

AC_SUBST(QT_CXXFLAGS)
AC_SUBST(QT_LDADD)
