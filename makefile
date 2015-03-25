# project compile environment
SHELL := /bin/sh
MAKE := make
CC := gcc
DB := gdb
AR := ar

# directory organization
TOP_DIR := $(PWD)

SRC_DIR := $(TOP_DIR)/src
BLD_DIR := $(TOP_DIR)/bld
BIN_DIR := $(TOP_DIR)/bin
DOC_DIR := $(TOP_DIR)/doc
MAKE_DIR := $(TOP_DIR)/make
TEST_DIR := $(TOP_DIR)/test
DIST_DIR := $(TOP_DIR)/dist
IMPORT_DIR := $(TOP_DIR)/import
EXPORT_DIR := $(TOP_DIR)/export
TEMPLATE_DIR := $(TOP_DIR)/template
RESOURCE_DIR := $(TOP_DIR)/resource

BLD_DEPEND_DIR := $(BLD_DIR)/depend
IMPORT_INC_DIR := $(IMPORT_DIR)/inc
IMPORT_LIB_DIR := $(IMPORT_DIR)/lib

# auxiliary makefiles
PACKAGE_MK := $(MAKE_DIR)/package.mk
IMPORT_MK := $(MAKE_DIR)/import.mk
CONFIG_MK := $(MAKE_DIR)/config.mk
TESTBENCH_MK := $(MAKE_DIR)/testbench.mk

# PACKAGE
sinclude $(PACKAGE_MK)
# IMPORT_INC_PATH IMPORT_LIB_PATH IMPORT_LIB
sinclude $(IMPORT_MK)
# CONFIG CONFIG_CFLAG
sinclude $(CONFIG_MK)
# TESTBENCH TESTARGS
sinclude $(TESTBENCH_MK)

BLD_OBJ_DIR := $(BLD_DIR)/$(CONFIG)
BIN_OUT_DIR := $(BIN_DIR)/$(CONFIG)
ifeq ($(strip $(TESTBENCH)),)
BIN_OUT := $(BIN_OUT_DIR)/$(notdir $(TOP_DIR)).out
else
BIN_OUT := $(BIN_OUT_DIR)/$(TESTBENCH).out
endif

# virtual paths
SRC_PATH := $(SRC_DIR)/
SRC_PATH += $(foreach p, $(PACKAGE), $(shell echo ": $(SRC_DIR)/$(p)"))
SRC_PATH += $(shell echo ": $(TEST_DIR)")

vpath %.c $(SRC_PATH)
vpath %.h $(SRC_PATH)
vpath %.o $(BLD_OBJ_DIR)/
vpath %.d $(BLD_DEPEND_DIR)/

INC_PATH := $(SRC_DIR)/
INC_PATH += $(foreach p, $(IMPORT_INC_PATH), $(shell echo ": $(p)"))

# file lists
SRC_LIST := $(notdir $(wildcard $(SRC_DIR)/*.c))
SRC_LIST += $(foreach p, $(PACKAGE), $(notdir $(wildcard $(SRC_DIR)/$(p)/*.c)))
SRC_LIST += $(foreach p, $(TESTBENCH), $(p).c)
OBJ_LIST := $(patsubst %.c, %.o, $(SRC_LIST))
DEPEND_LIST := $(patsubst %.o, %.d, $(OBJ_LIST))

# package list scan routine
define scan_package
mkdir -p $(SRC_DIR); \
mkdir -p $(MAKE_DIR); \
echo "make: scanning package list ..."; \
out=`ls -F $(SRC_DIR)/ | grep / | tr '\n' ' '`; \
cmp=; \
new="$$out"; \
while ! [ "$$cmp" == "$$out" ]; do \
    cmp="$$out"; \
    temp1=; \
    temp2=; \
    for p in $$new; do \
	temp1=`ls -F $(SRC_DIR)/$$p | grep / | tr '\n' ' '`; \
        for t in $$temp1;do \
            temp2="$$temp2 $$p$$t"; \
        done; \
    done; \
    new="$$temp2"; \
    out="$$out$$new"; \
done; \
echo "PACKAGE := $$out" > $(PACKAGE_MK); \
echo "make: package scan completed"; \
echo "make: package list: $$out"
endef

# pattern rules for building files
%.o: %.c
	@mkdir -p $(BLD_OBJ_DIR)
	@echo "make: building object file: $(notdir $<) -> $(notdir $@) ..."
	@$(CC) $(CONFIG_CFLAG) $(foreach p, $(INC_PATH), -I$(p)) \
		-c $< -o $(BLD_OBJ_DIR)/$@
	@echo "make: file $(notdir $@) build completed"

$(BLD_DEPEND_DIR)/%.d: %.c
	@mkdir -p $(BLD_DEPEND_DIR)
	@echo "make: building dependency: $(notdir $<) -> $(notdir $@) ..."
	@$(CC) $(foreach p, $(INC_PATH), -I$(p)) -MM $< > tmp.$$$$; \
	sed -e "s/: / $(subst /,\/,$@): /g" tmp.$$$$ > $@; \
	rm -rf tmp.$$$$
	@echo "make: file $(notdir $@) build completed"

all: $(BIN_OUT)

# automatic dependency
sinclude $(addprefix $(BLD_DEPEND_DIR)/, $(DEPEND_LIST))

# build executable
$(BIN_OUT): $(OBJ_LIST)
	@mkdir -p $(BIN_OUT_DIR)
	@echo "make: building output: $(notdir $(TOP_DIR)) -> $(notdir $@) ..."
	@$(CC) $(CONFIG_CFLAG) $(addprefix $(BLD_OBJ_DIR)/, $(OBJ_LIST)) \
		$(foreach p, $(IMPORT_LIB_PATH), -L$(p)) \
		$(foreach p, $(IMPORT_LIB), -l$(p)) -o $@
	@echo "make: file $(notdir $@) build completed"

# export header files
export_h:
	@mkdir -p $(EXPORT_DIR)/$(CONFIG)/inc
	@(cd $(SRC_DIR) && find . -name '*.h' | tar --create --files-from -) | \
		(cd $(EXPORT_DIR)/$(CONFIG)/inc && tar xvfp -)

# build shared library
so: $(OBJ_LIST) export_h
	@mkdir -p $(EXPORT_DIR)/$(CONFIG)/lib
	@echo "make: building shared library: lib$(notdir $(TOP_DIR)).so"
	@$(CC) -shared -Wall -O2 $(addprefix $(BLD_OBJ_DIR)/, $(OBJ_LIST)) \
		$(foreach p, $(IMPORT_LIB_PATH), -L$(p)) \
		$(foreach p, $(IMPORT_LIB), -l$(p)) \
		-o $(EXPORT_DIR)/$(CONFIG)/lib/lib$(notdir $(TOP_DIR)).so
	@echo "make: file lib$(notdir $(TOP_DIR)).so build completed"

# build static library
ar: $(OBJ_LIST) export_h
	@mkdir -p $(EXPORT_DIR)/$(CONFIG)/lib
	@echo "make: building static library: lib$(notdir $(TOP_DIR)).a"
	@$(AR) -rcs $(EXPORT_DIR)/$(CONFIG)/lib/lib$(notdir $(TOP_DIR)).a \
		$(addprefix $(BLD_OBJ_DIR)/, $(OBJ_LIST))
	@echo "make: file lib$(notdir $(TOP_DIR)).a build completed"

rebld: clean all

run: $(BIN_OUT)
	@echo "make: $(notdir $<) started >>>"
	@$(BIN_OUT) $(TESTARGS)
	@echo "make: $(notdir $<) stopped"

dbg: $(BIN_OUT)
	@$(DB) --args $(BIN_OUT) $(TESTARGS)

# toggle configuration
config: $(CONFIG_MK)
	@echo -n "make: select configuration: "; \
	read mode; \
	if [ "$$mode" == "" ] || [ $$mode == "show" ]; then \
		echo "make: CONFIG := $(CONFIG)"; \
	else \
		tmp=`grep $$mode $(CONFIG_MK)`; \
		if [ "$$tmp" == "" ]; then \
			echo "make: mode not supported"; \
			exit 0; \
		fi; \
		if ! [ -f $(CONFIG_SRC).$(CONFIG) ] && [ -f $(CONFIG_SRC) ]; then \
			mv $(CONFIG_SRC) $(CONFIG_SRC).$(CONFIG); \
		fi; \
		if ! [ -f $(CONFIG_SRC) ] && [ -f $(CONFIG_SRC).$$mode ]; then \
			mv $(CONFIG_SRC).$$mode $(CONFIG_SRC); \
		fi; \
		sed "s/CONFIG :.*/CONFIG := $$mode/g" $(CONFIG_MK) > tmp.$$$$; \
		mv tmp.$$$$ $(CONFIG_MK); \
		echo "make: CONFIG := $$mode"; \
	fi

$(CONFIG_MK):
	@mkdir -p $(MAKE_DIR)
	@echo "CONFIG := debug" > $(CONFIG_MK)
	@echo "debug_CFLAG := -Wall -g" >> $(CONFIG_MK)
	@echo "release_CFLAG := -Wall -O2" >> $(CONFIG_MK)
	@echo "CONFIG_CFLAG := #(#(CONFIG)_CFLAG)" >> $(CONFIG_MK)
	@echo "CONFIG_SRC := none" >> $(CONFIG_MK)
	@sed "s/#/$$/g"  $(CONFIG_MK) > tmp.$$$$; \
	mv tmp.$$$$ $(CONFIG_MK)

# package management
package:
	@echo -n "make: input new package names/paths: "
	@read pack; \
	mkdir -p $(SRC_DIR)/$$pack; \
	echo "make: package $$pack created"
	@$(call scan_package)

package_scan:
	@$(call scan_package)

package_del:
	@echo -n "make: input package names/paths: "
	@read pack; \
	echo -n "make: all src in $$pack will be deleted; continue?[y/n]"; \
	read confirm; \
	if  [ "$$confirm" == "y" ]; then \
	    rm -rf $(SRC_DIR)/$$pack; \
	    echo "make: package $$pack deleted"; \
	fi; \
	@$(call scan_package)

# import management
# this file may be edited manually
$(IMPORT_MK):
	@mkdir -p $(MAKE_DIR)
	@echo "IMPORT_LIB :=" > $(IMPORT_MK)
	@echo "IMPORT_LIB_PATH := $(IMPORT_LIB_DIR)/" >> $(IMPORT_MK)
	@echo "IMPORT_INC_PATH := $(IMPORT_INC_DIR)/" >> $(IMPORT_MK)

# generate sources from template
src:
	@mkdir -p $(SRC_DIR)
	@mkdir -p $(TEMPLATE_DIR)
	@echo -n "make: input package name: "; \
	read pack; \
	echo -n "make: input src stereotype: "; \
	read type; \
	if [ "$$type" == "" ]; then \
		type="not_exist_$$$$"; \
	fi; \
	echo -n "make: input src name: "; \
	read name; \
	if [ "$$name" == "" ]; then \
		echo "make: invalid src name"; \
		exit 0; \
	fi; \
	lpack=`echo $$pack | tr '[/]' '[_]'`; \
	upack=`echo $$pack | tr '[a-z/]' '[A-Z_]'`; \
	utype=`echo $$type | tr '[a-z]' '[A-Z]'`; \
	uname=`echo $$name | tr '[a-z]' '[A-Z]'`; \
	cfile=`ls $(TEMPLATE_DIR)/$$type.* 2> /dev/null | wc -l`; \
	if [ $$cfile == "0" ]; then \
		echo "make: template not found, use empty file"; \
		echo "" > $(SRC_DIR)/$$pack/$$name.c; \
	else \
		temps=`ls $(TEMPLATE_DIR)/$$type.* | tr '\n' ' '`; \
		for t in $$temps; do \
		    ext=`echo "$$t" | awk -F '.' '{print $$2}'`; \
		    uext=`echo $$ext | tr '[a-z]' '[A-Z]'`; \
		    echo "make: template $$type.$$ext -> $$name.$$ext ..."; \
		    mkdir -p $(SRC_DIR)/$$pack; \
		    sed -e "s/%type/$$type/g" -e "s/%pack/$$lpack/g" \
			-e "s/%name/$$name/g" -e "s/%ext/$$ext/g" \
			-e "s/%TYPE/$$utype/g" -e "s/%PACK/$$upack/g" \
			-e "s/%NAME/$$uname/g" -e "s/%EXT/$$uext/g" \
			-e "s/%prj/$(notdir $(TOP_DIR))/g" \
			$$t > $(SRC_DIR)/$$pack/$$name.$$ext; \
		    echo "make: source file $$name.$$ext created"; \
		done; \
	fi
	@$(call scan_package)

# testbench
tb:
	@mkdir -p $(TEST_DIR)
	@echo -n "make: input testbench name: "; \
	read tbname; \
	if [ "$$tbname" == "" ]; then \
		echo "make: reset testbench"; \
	elif [ "$$tbname" == "show" ]; then \
		echo "TESTBENCH :=" $(TESTBENCH); \
		exit 0; \
	elif ! [ -f $(TEST_DIR)/$$tbname.c ]; then \
		if ! [ -f $(TEMPLATE_DIR)/main.c ]; then \
			echo "make: main template not found, use empty file"; \
			echo "" > $(TEST_DIR)/$$tbname.c; \
		else \
			cp $(TEMPLATE_DIR)/main.c $(TEST_DIR)/$$tbname.c; \
		fi; \
	fi; \
	sed "s/TESTBENCH :.*/TESTBENCH := $$tbname/g" $(TESTBENCH_MK) > tmp.$$$$; \
	mv tmp.$$$$ $(TESTBENCH_MK); \
	echo "make: TESTBENCH := $$tbname"

$(TESTBENCH_MK):
	@mkdir -p $(MAKE_DIR)
	@echo "TESTBENCH :=" > $(TESTBENCH_MK)
	@echo "TESTARGS :=" >> $(TESTBENCH_MK)

$(PACKAGE_MK):
	@$(call scan_package)

# clean up
clean:
	@echo make: "clean up bld files ..."
	@cd $(BLD_DEPEND_DIR); \
	rm -rf ./*.d
	@cd $(BLD_OBJ_DIR); \
	rm -rf ./*.o
	@cd $(BIN_OUT_DIR); \
	rm -rf $(BIN_OUT)
	@echo "make: bld cleaned"

show: 
	@echo $(SRC_DIR) 

.PHONY: all rebld run dbg config package package_scan package_del src tb clean
