# Baby Makefile for Linux
#

CONFIG=debug
#CONFIG=release

NAME=corelib

# Following not used in Linux build
NOTSRC          = PWSclipboard.cpp

LIBSRC          = sha1.cpp sha256.cpp BlowFish.cpp hmac.cpp \
		              TwoFish.cpp PWCharPool.cpp PWSdirs.cpp \
	                PWSrand.cpp SysInfo.cpp Util.cpp StringX.cpp \
									ItemField.cpp UUIDGen.cpp Debug.cpp PWSprefs.cpp \
									ItemData.cpp PWSfile.cpp PWHistory.cpp \
									PWSfileV1V2.cpp PWSfileV3.cpp \
									PWSFilters.cpp \
									UnknownField.cpp  VerifyFormat.cpp Match.cpp \
									UTF8Conv.cpp Report.cpp PWScore.cpp \
									XMLprefs.cpp tinyxml/tinyxml.cpp \
									tinyxml/tinyxmlerror.cpp tinyxml/tinyxmlparser.cpp \
									XML/XMLFileHandlers.cpp XML/XMLFileValidation.cpp \
									XML/XMLUtilities.cpp \
									XML/Expat/EFileHandlers.cpp XML/Expat/EFileValidator.cpp \
									XML/Expat/EFileXMLProcessor.cpp XML/Expat/EFilterHandlers.cpp \
									XML/Expat/EFilterValidator.cpp XML/Expat/EFilterXMLProcessor.cpp \
									XML/Expat/ESecMemMgr.cpp \
									XML/Xerces/XFileSAX2Handlers.cpp XML/Xerces/XFileValidator.cpp \
									XML/Xerces/XFileXMLProcessor.cpp XML/Xerces/XFilterSAX2Handlers.cpp \
									XML/Xerces/XFilterXMLProcessor.cpp XML/Xerces/XSecMemMgr.cpp

SRC             = $(LIBSRC)

OBJPATH         = ../../obj/$(CONFIG)/$(NAME)
LIBPATH         = ../../lib/$(CONFIG)

#destination related macros
LIBOBJ	  = $(addprefix $(OBJPATH)/,$(subst .cpp,.o,$(LIBSRC)))
LIB	     = $(LIBPATH)/libcore.a
OBJ	     = $(LIBOBJ)

CXXFLAGS +=  -Wall -I..

ifeq ($(CONFIG),debug)
CXXFLAGS += -O0 -ggdb
else ifeq ($(CONFIG),release)
CXXFLAGS += -O
endif

# rules
.PHONY: all debug release clean setup

$(OBJPATH)/%.o : %.c
	$(CC) -g  $(CFLAGS)   -c $< -o $@

$(OBJPATH)/%.o : %.cpp
	$(CXX) -g $(CXXFLAGS) -c $< -o $@

all : setup $(LIB)

debug release:
	$(MAKE) all CONFIG=$@

$(LIB): $(LIBOBJ)
	ar rvus $@ $(filter %.o,$^)

clean:
	rm -f *~ $(OBJ) $(TEST) $(LIB) $(DEPENDFILE)

setup:
	@mkdir -p $(OBJPATH) $(OBJPATH)/tinyxml $(OBJPATH)/XML/Expat $(OBJPATH)/XML/Xerces $(LIBPATH)
