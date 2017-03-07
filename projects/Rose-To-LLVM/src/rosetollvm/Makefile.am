include $(top_srcdir)/config/Makefile.for.ROSE.includes.and.libs

AM_LDFLAGS = $(ROSE_LIBS) $(LIBS_WITH_RPATH) 
AM_CPPFLAGS = $(ROSE_INCLUDES) -I$(srcdir)/include -Wno-deprecated -Wall -Wextra -g -O0 

pkginclude_HEADERS 	= \
	include/rosetollvm/RootAstAttribute.h \
	include/rosetollvm/ManagerAstAttribute.h \
	include/rosetollvm/AggregateAstAttribute.h \
	include/rosetollvm/AttributesAstAttribute.h \
	include/rosetollvm/CaseAstAttribute.h \
	include/rosetollvm/CodeAttributesVisitor.h \
	include/rosetollvm/CodeEmitter.h \
	include/rosetollvm/CodeGeneratorVisitor.h \
	include/rosetollvm/ConditionalAstAttribute.h \
	include/rosetollvm/ConditionalComponentAstAttribute.h \
	include/rosetollvm/ConstantFolder.h \
	include/rosetollvm/DeclarationsAstAttribute.h \
	include/rosetollvm/DoAstAttribute.h \
	include/rosetollvm/ForAstAttribute.h \
	include/rosetollvm/FunctionAstAttribute.h \
	include/rosetollvm/IfAstAttribute.h \
	include/rosetollvm/IfComponentAstAttribute.h \
	include/rosetollvm/IntAstAttribute.h \
	include/rosetollvm/LLVMAstAttributes.h \
	include/rosetollvm/LLVMVisitor.h \
	include/rosetollvm/LogicalAstAttribute.h \
	include/rosetollvm/Option.h \
	include/rosetollvm/Control.h \
	include/rosetollvm/RoseToLLVM.hpp \
	include/rosetollvm/SgTypeAstAttribute.h \
	include/rosetollvm/SizeOfAstAttribute.h \
	include/rosetollvm/StringAstAttribute.h \
	include/rosetollvm/StringSet.h \
	include/rosetollvm/SwitchAstAttribute.h \
	include/rosetollvm/WhileAstAttribute.h

lib_LTLIBRARIES = librosetollvm.la

librosetollvm_la_SOURCES = \
	RoseToLLVM.cpp \
	CodeAttributesVisitor.cpp \
	CodeEmitter.cpp \
	CodeGeneratorVisitor.cpp \
	ConstantFolder.cpp \
	LLVMAstAttributes.cpp \
	Option.cpp \
	Control.cpp \
	StringSet.cpp

librosetollvm_la_CPPFLAGS	= \
	-I$(srcdir)/include \
	$(LLVM_CPPFLAGS) 

librosetollvm_la_LDFLAGS = $(ROSE_LIBS) $(LLVM_LDFLAGS)

librosetollvm_la_CXXFLAGS = -Wno-deprecated 

clean-local:
	rm -rf *.lo .libs 
