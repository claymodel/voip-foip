# $Id$
#
# sip_router makefile
#
# WARNING: requires gmake (GNU Make)
#  Arch supported: Linux, FreeBSD, SunOS (tested on Solaris 8), OpenBSD (3.2),
#  NetBSD (1.6).
#
#  History:
#  --------
#              created by andrei
#  2003-02-24  make install no longer overwrites ser.cfg  - patch provided
#               by Maxim Sobolev   <sobomax@FreeBSD.org> and 
#                  Tomas Björklund <tomas@webservices.se>
#  2003-03-11  PREFIX & LOCALBASE must also be exported (andrei)
#  2003-04-07  hacked to work with solaris install (andrei)
#  2003-04-17  exclude modules overwritable from env. or cmd. line,
#               added include_modules and skip_modules (andrei)
#  2003-05-30  added extra_defs & EXTRA_DEFS
#               Makefile.defs force-included to allow recursive make
#               calls -- see comment (andrei)
#  2003-06-02  make tar changes -- unpacks in $NAME-$RELEASE  (andrei)
#  2003-06-03  make install-cfg will properly replace the module path
#               in the cfg (re: /usr/.*lib/ser/modules)
#              ser.cfg.default is installed only if there is a previous
#               cfg. -- fixes packages containing ser.cfg.default (andrei)
#  2003-08-29  install-modules-doc split from install-doc, added 
#               install-modules-all, removed README.cfg (andrei)
#              added skip_cfg_install (andrei)
#  2004-09-02  install-man will automatically "fix" the path of the files
#               referred in the man pages
#  2006-02-14  added utils & install-utils (andrei)
#  2006-03-15  added nodeb parameter for make tar (andrei)
#  2006-09-29  added modules-doc as target and doc_format= as make option (greger)
#  2006-12-09  added new group_include as make option and defined groups 
#               defining which modules to include. Also added new target 
#               print-modules that you can use to check which modules will be 
#               compiled (greger)
#  2007-01-10  added new group_include targets mysql, radius, and presence 
#               improved print-modules output fixed problem in include/exclude
#               logic when using group_include (greger)
#  2007-03-01  fail if a module or a required utility make fail unless 
#              err_fail=0; don't try to make modules with no Makefiles (andrei)
#  2007-03-16  moved the exports to Makefile.defs (andrei)
#  2007-03-29  install-modules changed to use make -C modpath install (andrei)
#  2007-05-04  "if ! foo" not supported in standard sh, switched to 
#                "if foo; then :; else ... ; fi" (andrei)
#  2008-06-23  added 2 new targets: README and man (re-generate the README
#              or manpages for all the modules) (andrei)
#  2008-06-25  make cfg support (use a pre-built cfg.: config.mak) (andrei)
#  2008-06-28  added clean-all, proper-all, install-modules-man and error 
#               checks for install-utils & doc (andrei)
#  2008-07-01  split module list from config.mak into modules.lst so that
#               the modules list can be changed without rebuilding the whole
#               ser (andrei)
#              added cfg-defs, new target that only rebuilds config.mak
#  2009-03-10  replaced DEFS with C_DEFS (DEFS are now used only for
#              "temporary" defines inside modules or libs) (andrei)
#  2009-03-27  multiple modules directory support, see modules_dirs (andrei)
#  2009-04-02  workaround for export not supported in gnu make 3.80
#               target specific variables: use mk_params for each
#               $(MAKE) invocation (andrei)
#  2009-04-22  don't rebuild config.mak or modules.lst if not needed
#              (e.g. on clean) (andrei)
#  2009-06-24  auto-generate autover.h, containing the REPO_VER macro, defined
#               to the top git commit sha (if git is found) (andrei)
#

# check make version
# everything works with 3.80, except evals inside ifeq/endif
# (see https://savannah.gnu.org/bugs/index.php?func=detailitem&item_id=1516).
# recommended 3.81
req_ver=3.81
# the check below works for version number of the type x.yy or x.yy.z*
# (from the GNU Make Cookbook)
ifeq (,$(filter $(req_ver),$(firstword $(sort $(MAKE_VERSION) $(req_ver)))))
$(error make version $(MAKE_VERSION) not supported, use at least $(req_ver))
endif


auto_gen=lex.yy.c cfg.tab.c #lexx, yacc etc
auto_gen_others=cfg.tab.h # auto generated, non-c
auto_gen_keep=autover.h # auto generated, should be included in archives

COREPATH=.
#include  source related defs
include Makefile.sources
#include special targets lists
include Makefile.targets

# whether or not the entire build process should fail if building a module or
#  an utility fails
err_fail?=1

# whether or not to install $(MAIN_NAME).cfg or just $(MAIN_NAME).cfg.default
# ($(MAIN_NAME).cfg will never be overwritten by make install, this is usefull
#  when creating packages)
skip_cfg_install?=

#extra modules to exclude
skip_modules?=

# see Makefile.dirs for the directories used for the modules
include Makefile.dirs

# Set document format
# Alternatives are txt, html, xhtml, and pdf (see Makefile.doc)
doc_format?=html

# don't force modules.lst generation if the makefile goals do not
# require it (but if present use it)
ifeq (,$(strip $(filter-out $(clean_targets) $(aux_targets),$(MAKECMDGOALS))))
ifneq (,$(strip $(wildcard modules.lst)))
-include modules.lst
endif
else
include modules.lst
endif # ifneq (,$(strip $(filter-out ...,$(MAKECMDGOALS))))

#if called with group_include, ignore the modules from modules.lst
ifneq ($(group_include),)
	include_modules=
	exclude_modules=
	modules_configured:=0
endif

# Module group definitions, default only include the standard group
# Make backwards compatible, don't set group_include default...
#group_include?="standard"

# Modules in this group are considered a standard part of SER (due to 
# widespread usage) and have no external compile or link dependencies (note 
# that some of these interplay with external systems).
module_group_standard=acc_syslog auth avp ctl dispatcher diversion enum\
				eval exec fifo db_flatstore gflags maxfwd mediaproxy \
				nathelper options pdt permissions pike print ratelimit \
				registrar rr rtpproxy sanity sl textops timer tm uac \
				unixsock uri usrloc xlog cfg_rpc

# Modules in this group are considered a standard part of SER (due to 
# widespread usage) but they have dependencies that must be satisfied for 
# compilation.
# acc_radius, auth_radius, misc_radius => radiusclient-ng
# acc_db, auth_db, avp_db, db_ops, domain, lcr, msilo, dialog, speeddial,
# uri_db => database module (db_mysql, db_postgres, dbtext ...)
# mysql, postgres => mysql server and client libraries or postgres server and
#  client libraries or other database back-end (ex. mysql-devel)
# pa, xmlrpc => libxml2
# rls => pa
#
# NOTE! All presence modules (dialog, pa, presence_b2b, rls, xcap) have been
# included in this group due to interdependencies
module_group_standard_dep=acc_db acc_radius auth_db auth_radius avp_db \
				auth_identity db_ops domain lcr misc_radius \
				msilo dialog pa \
				presence_b2b rls speeddial uri_db xcap xmlrpc

# For db use (db modules, excluding drivers)
module_group_db=acc_db auth_db avp_db db_ops db_text \
				uri_db domain lcr msilo speeddial
				#dbtext (s) not migrated yet to the new db interface

# For mysql
module_group_mysql_driver=db_mysql
module_group_mysql=$(module_group_mysql_driver) $(module_group_db)

# For postgress
module_group_postgres_driver=db_postgres
module_group_postgres=$(module_group_postgres_driver) $(module_group_db)

# For radius
module_group_radius=acc_radius auth_radius misc_radius avp_radius uri_radius \
					peering

# For presence
# kamailio modules
module_group_presence=presence presence_dialoginfo presence_mwi presence_xml \
						pua pua_bla pua_dialoginfo pua_mi pua_usrloc pua_xmpp \
						rls xcap_client xcap_server presence_conference
#ser modules
module_group_presence+=dialog presence_b2b xcap
# obsolete/unmaintained ser modules
#module_group_presence=pa rls

# Modules in this group satisfy specific or niche applications, but are 
# considered stable for production use. They may or may not have dependencies
# cpl-c => libxml2
# jabber => expat (library)
# osp => OSP Toolkit (sipfoundry)
# sms => none (external modem)
module_group_stable=cpl-c dbtext jabber osp sms pdb

# Modules in this group are either not complete, untested, or without enough
# reports of usage to allow the module into the stable group. They may or may
# not have dependencies
module_group_experimental=tls oracle iptrtpproxy

# Kamailio specific groups
# Standard modules in K Debian distro
module_group_kstandard=acc alias_db auth auth_db benchmark call_control \
				cfgutils db_text dialog dispatcher diversion domain drouting \
				exec group htable imc kex maxfwd mi_datagram mi_fifo msilo \
				nat_traversal nathelper path pdt permissions pike pv qos \
				ratelimit regex registrar rr rtimer rtpproxy siptrace siputils \
				sl sms speeddial sqlops sst statistics textops tmx uac \
				uac_redirect uri_db userblacklist usrloc xlog seas \
				avpops cfg_db cfg_rpc ctl db_flatstore dialplan enum \
				iptrtpproxy lcr mediaproxy mi_rpc pdb sanity tm topoh \
				blst prefix_route counters debugger matrix mqueue mtree \
				pipelimit rtpproxy textopsx xhttp

# K mysql module
module_group_kmysql=db_mysql

# K postgress module
module_group_kpostgres=db_postgres

# K cpl module
module_group_kcpl=cpl-c

# K radius modules
module_group_kradius=acc_radius auth_radius misc_radius peering

# K unixodbc module
module_group_kunixodbc=db_unixodbc

# K xmlrpc modules
module_group_kxml=xmlrpc mi_xmlrpc xmlops

# K perl module
module_group_kperl=perl perlvdb

# K snmpstats module
module_group_ksnmpstats=snmpstats

# K xmpp module
module_group_kxmpp=xmpp

# K carrierroute module
module_group_kcarrierroute=carrierroute

# K berkeley module
module_group_kberkeley=db_berkeley

# K ldap modules
module_group_kldap=ldap h350

# K utils module
module_group_kutils=utils

# K purple module
module_group_kpurple=purple

# K memcached module
module_group_kmemcached=memcached

# K tls module
module_group_ktls=tls

# K presence modules
module_group_kpresence=presence presence_dialoginfo presence_mwi presence_xml \
						pua pua_bla pua_dialoginfo pua_mi pua_usrloc pua_xmpp \
						rls xcap_client xcap_server presence_conference

# K lua module
module_group_klua=app_lua

# K python module
module_group_kpython=app_python

# K geoip module
module_group_kgeoip=geoip

# if not set on the cmd. line, env or in the modules.lst (cfg_group_include)
# exclude the below modules.
ifneq ($(group_include)$(cfg_group_include),)
	# For group_include, default all modules are excluded except those in 
	# include_modules
	exclude_modules?=
else
	# Old defaults for backwards compatibility
	# excluded because they depend on external libraries
	exclude_modules?= 		cpl mangler postgres jabber mysql cpl-c \
							auth_radius misc_radius avp_radius uri_radius \
							acc_radius pa rls presence_b2b xcap xmlrpc\
							osp tls oracle \
							unixsock dbg print_lib auth_identity ldap \
							db_berkeley db_mysql db_postgres db_oracle \
							db_unixodbc memcached mi_xmlrpc \
							perl perlvdb purple \
							snmpstats xmpp \
							carrierroute peering \
							dialplan lcr utils presence presence_mwi \
							presence_dialoginfo presence_xml pua pua_bla \
							pua_dialoginfo pua_usrloc pua_xmpp \
							regex xcap_client xcap_server presence_conference
	#excluded because they depend on external *.h files
	exclude_modules+= h350
	# excluded because they do not compile (remove them only after they are
	#  fixed) -- andrei
	exclude_modules+= bdb dbtext iptrtpproxy pa rls
	# depends on libgeoip
	exclude_modules+= geoip
	# depends on liblua5.1-dev
	exclude_modules+= app_lua
	# depends on libpython-dev
	exclude_modules+= app_python
	# depends on libxml2
	exclude_modules+= xmlops
	# depends on tm being compiled with -DWITH_AS_SUPPORT support
ifeq (,$(findstring -DWITH_AS_SUPPORT, $(C_DEFS)))
		exclude_modules+= seas
endif
endif

# always exclude the CVS dir
override exclude_modules+= CVS $(skip_modules)

# Test for the groups and add to include_modules
ifneq (,$(group_include))
$(eval override include_modules+= $(foreach grp, $(group_include), \
										$(module_group_$(grp)) ))
endif

# first 2 lines are excluded because of the experimental or incomplete
# status of the modules
# the rest is excluded because it depends on external libraries
#
static_modules:=

ALLDEP=config.mak Makefile Makefile.dirs Makefile.sources Makefile.rules

#include general defs (like CC, CFLAGS  a.s.o)
# hack to force makefile.defs re-inclusion (needed when make calls itself with
# other options -- e.g. make bin)
#makefile_defs=0
#C_DEFS:=


# try saved cfg, unless we are in the process of building it or if we're doing
# a clean
ifeq (,$(strip \
	$(filter config.mak config cfg cfg-defs $(clean_targets),$(MAKECMDGOALS))))
include config.mak
ifeq ($(makefile_defs),1)
ifeq ($(quiet),verbose)
$(info config.mak loaded)
endif # verbose
export makefile_defs
# config_make valid & used
config_mak=1
ifeq ($(MAIN_NAME),)
$(error "bad config.mak, try re-running make cfg")
endif
endif
else # config.mak doesn't need to be used
ifneq (,$(filter cfg config cfg-defs,$(word 1,$(MAKECMDGOALS))))
# needed here to avoid starting a config submake 
# (e.g. rm -f config.mak; make config.mak), which would either require 
# double Makefile.defs defines execution (suboptimal), would loose
# $(value ...) expansion or would cause some warning (if Makefile.defs exec. 
# is skipped in the "main" makefile invocation).
$(shell rm -rf config.mak)
config_mak=0
makefile_defs=0
exported_vars=0
else
# config.mak not strictly needed, but try to load it if exists for $(Q)
config_mak=skip
-include config.mak
export makefile_defs
endif
endif


main_makefile=1
include Makefile.defs

static_modules_path=$(addprefix modules/, $(static_modules))
extra_sources=$(wildcard $(addsuffix /*.c, $(static_modules_path)))
extra_objs=$(extra_sources:.c=.o)

static_defs:= $(foreach  mod, $(static_modules), \
		-DSTATIC_$(shell echo $(mod) | tr [:lower:] [:upper:]) )

override extra_defs+=$(static_defs) $(EXTRA_DEFS)
export extra_defs

# Historically, the resultant set of modules is: modules/* - exclude_modules +
# include_modules
# When group_include is used, we want: include_modules (based on group_include)
# - exclude_modules

ifneq ($(modules_configured),1) 
#modules_all=$(filter-out modules/CVS,$(wildcard modules/*))

# create modules*_all vars
$(foreach mods,$(modules_dirs),$(eval \
	$(mods)_all=$$(filter-out $(mods)/CVS,$$(wildcard $(mods)/*))))
#debugging
#$(foreach mods,$(modules_dirs),$(info "$(mods)_all=$($(mods)_all)"))

ifneq ($(group_include),)
$(foreach mods,$(modules_dirs),$(eval \
	$(mods)=$$(filter-out $$(addprefix $(mods)/, \
			$$(exclude_modules) $$(static_modules)), \
			$$(addprefix $(mods)/, $$(include_modules) )) ))
else	
	# Standard, old resultant set
$(foreach mods,$(modules_dirs),$(eval \
	$(mods)_noinc=$$(filter-out $$(addprefix $(mods)/, \
			$$(exclude_modules) $$(static_modules)), $$($(mods)_all)) \
))
$(foreach mods,$(modules_dirs),$(eval \
	$(mods)=$$(filter-out $$(modules_noinc), \
			$$(addprefix $(mods)/, $$(include_modules) )) $$($(mods)_noinc) \
))
endif # ifneq($(group_include),)
endif # ifneq($(modules_configured),1)

$(foreach mods,$(modules_dirs),$(eval \
	$(mods)_names=$$(shell echo $$($(mods))| \
				sed -e "s/$(mods)"'\/\([^/ ]*\)\/*/\1.so/g' ) \
))
$(foreach mods,$(modules_dirs),$(eval \
	$(mods)_basenames:=$$(shell echo $$($(mods))| \
				sed -e "s/$(mods)"'\/\([^/ ]*\)\/*/\1/g' ) \
))

# all modules from all the $(modules_dirs)
all_modules_lst=$(foreach mods,$(modules_dirs), $($(mods)_all))

# compile modules list (all the compiled mods from  $(modules_dirs))
cmodules=$(foreach mods,$(modules_dirs), $($(mods)))

#modules_names=$(patsubst modules/%, %.so, $(modules))
#modules_full_path=$(join  $(modules), $(addprefix /, $(modules_names)))



# list of utils directories that should be compiled by make utils
C_COMPILE_UTILS=	utils/sercmd
# list of binaries that should be installed alongside
# (they should be created after make utils, see C_COMPILE_UTILS)
C_INSTALL_BIN=	# sercmd is now installed by ctl

# which utils know to install themselves and should be installed
# along the core (list of utils directories)
ifeq ($(FLAVOUR),kamailio)
C_INSTALL_UTILS= utils/kamctl
else
C_INSTALL_UTILS=
endif
# list of scripts that should be installed along the core 
# (here a script is something that doesn't have a Makefile)
C_INSTALL_SCRIPTS=
# list of extra configs that should be installed along the core
# Note: all the paths of the form /usr/*lib/$(CFG_NAME)/<module_dir>
# will be updated to the directory where the modules will be installed.
C_INSTALL_CFGS=
# list of files that should be installed in the arch-independent 
# directory (by default /usr/local/share/$(MAIN_NAME)))
C_INSTALL_SHARE=




NAME=$(MAIN_NAME)

tar_name=$(NAME)-$(RELEASE)_src

tar_extra_args+=$(addprefix --exclude=$(notdir $(CURDIR))/, \
					$(auto_gen) $(auto_gen_others))
ifeq ($(CORE_TLS), 1)
	tar_extra_args+=
else
	tar_extra_args+=--exclude=$(notdir $(CURDIR))/tls/* 
endif

ifneq ($(nodeb),)
	tar_extra_args+=--exclude=$(notdir $(CURDIR))/debian 
	tar_name:=$(tar_name)_nodeb
endif

# sanity checks
ifneq ($(TLS),)
        $(warning "make TLS option is obsoleted, try TLS_HOOKS or CORE_TLS")
endif

# include the common rules
include Makefile.rules

#extra targets 

$(NAME): $(extra_objs) # static_modules

lex.yy.c: cfg.lex cfg.tab.h $(ALLDEP)
	$(LEX) $<

cfg.tab.c cfg.tab.h: cfg.y  $(ALLDEP)
	$(YACC) $(YACC_FLAGS) $<

nullstring=
space=$(nullstring) $(nullstring)

modules_search_path=$(subst $(space),:,$(strip\
						$(foreach m,$(modules_dirs),$($(m)_target))))
		#				$(addprefix $(modules_target),$(modules_dirs))))

# special depends for main.o
main.o: DEFS+=-DMODS_DIR='"$(modules_search_path)"'


#special depends for ver.c
ver.d ver.o: autover.h

include Makefile.shared

ifeq ($(config_mak),1)

include Makefile.cfg

# fix basedir path (relative -> absolute)
ifneq (,$(basedir))
ifeq (,$(filter /%, $(basedir)))
override basedir:=$(CURDIR)/$(basedir)
# remove basedir from command line overrides
MAKEOVERRIDES:=$(filter-out basedir=%,$ $(MAKEOVERRIDES))
endif # (,$(filter /%, $(basedir)))
endif # (,$(basedir))

else ifneq ($(config_mak),skip)

config.mak: Makefile.defs
	@echo making config...
	@echo "# this file is autogenerated by make cfg" >$@
	@$(call mapf2,cfg_save_var,saved_fixed_vars,$(@))
	@$(call mapf2,cfg_save_var2,saved_chg_vars,$(@))
	@echo "override makefile_defs:=1" >>$@
	@echo "C_DEFS:=\$$(filter-out \$$(DEFS_RM) \$$(extra_defs),\$$(C_DEFS))" \
					"\$$(extra_defs)"  >>$@
	@echo "CFLAGS:=\$$(filter-out \$$(CFLAGS_RM) \$$(CC_EXTRA_OPTS)," \
						"\$$(CFLAGS)) \$$(CC_EXTRA_OPTS)" >>$@

endif # ifeq ($(config_mak),1)

modules.lst:
	@echo  saving modules list...
	@echo "# this file is autogenerated by make modules-cfg" >$@
	@echo "modules_dirs:=$(modules_dirs)" >>$@
	@echo "cfg_group_include=$(group_include)" >>$@
	@$(call cfg_save_var2,include_modules,$@)
	@$(call cfg_save_var2,static_modules,$@)
	@$(call cfg_save_var2,skip_modules,$@)
	@$(call cfg_save_var2,exclude_modules,$@)
	@$(foreach mods,$(modules_dirs), \
		$(call cfg_save_var2,$(mods)_all,$@))
	@$(foreach mods,$(modules_dirs), \
		$(call cfg_save_var2,$(mods)_noinc,$@))
	@$(foreach mods,$(modules_dirs), \
		$(call cfg_save_var2,$(mods),$@))
	@echo "modules_configured:=1" >>$@


.PHONY: cfg config cfg-defs
cfg-defs: config.mak

cfg config: cfg-defs modules-cfg

.PHONY: modules-cfg modules-list modules-lst
modules-cfg modules-list modules-lst:
	rm -f modules.lst
	$(MAKE) modules.lst

ifneq ($(wildcard .git),)
# if .git/ exists
repo_ver=$(shell  RV=`git rev-parse --verify --short=6 HEAD 2>/dev/null`;\
					[ -n "$$RV" ] && \
					test -n "`git update-index --refresh --unmerged >/dev/null\
							; git diff-index --name-only HEAD 2>/dev/null | \
								grep -v Makefile`" &&\
						RV="$$RV"-dirty; echo "$$RV")
repo_hash=$(subst -dirty,,$(repo_ver))
repo_state=$(subst %-dirty,dirty,$(findstring -dirty,$(repo_ver)))
autover_h_dep=.git $(filter-out $(auto_gen), $(sources)) cfg.y cfg.lex Makefile
else
# else if .git/ does not exist
repo_ver=
repo_hash="unknown"
repo_state=
autover_h_dep=
endif


autover.h: $(autover_h_dep)
	@echo  "generating autover.h ..."
	@echo "/* this file is autogenerated by make autover.h" >$@
	@echo " * DO NOT EDIT IT" >>$@
	@echo " */" >>$@
	@echo "" >>$@
	@echo "#define REPO_VER \"$(repo_ver)\"" >>$@
	@echo "#define REPO_HASH \"$(repo_hash)\"" >>$@
	@echo "#define REPO_STATE \"$(repo_state)\"" >>$@

.PHONY: all
all: $(NAME) every-module

.PHONY: print-modules
print-modules:
	@echo The following modules were chosen to be included: \
		$(include_modules) ; \
	echo ---------------------------------------------------------- ; \
	echo The following modules will be excluded: $(exclude_modules) ; \
	echo ---------------------------------------------------------- ; \
	echo The following modules will be made; \
	$(foreach mods,$(modules_dirs), \
		echo $(mods)/: $($(mods)_basenames) ; ) \
	#echo DBG: The following modules will be made: $(modules_basenames) ; \


# modules templates (instantiated based on modules_dirs contents)
define MODULES_RULES_template

$(1)_dst=$(modules_prefix)/$(modules_dir)$(1)
$(1)_target=$(prefix)/$(modules_dir)$(1)

.PHONY: $(1)
$(1): modules.lst
	@$(foreach r,$($(1)),$(call module_make,$(r),$(mk_params)))

.PHONY: $(1)-doc
$(1)-doc: modules.lst
	+@for r in $($(1)) "" ; do \
		if [ -n "$$$$r" -a -r "$$$$r/Makefile" ]; then \
			$(call oecho, "" ;) \
			$(call oecho, "" ;) \
			$(MAKE) -C $$$$r/doc $(doc_format) $$(mk_params); \
		fi ; \
	done

.PHONY: $(1)-readme

$(1)-readme: modules.lst
	-+@for r in $($(1)) "" ; do \
		if [ -n "$$$$r" -a -r "$$$$r/Makefile" ]; then \
			$(call oecho, "" ;) \
			$(call oecho, "" ;) \
			if  $(MAKE) -C $$$$r $$(mk_params) README || [ ${err_fail} != 1 ];\
			then \
				:; \
			else \
				exit 1; \
			fi ; \
		fi ; \
	done; true

.PHONY: $(1)-man
$(1)-man: modules.lst
	-+@for r in $($(1)_basenames) "" ; do \
		if [ -n "$$$$r" -a -r $(1)/"$$$$r/Makefile" -a \
			 -r $(1)/"$$$$r/$$$$r.xml" ]; then \
			$(call oecho, "" ;) \
			$(call oecho, "" ;) \
			if  $(MAKE) -C $(1)/"$$$$r" $$(mk_params) man || \
				[ ${err_fail} != 1 ] ;\
			then \
				:; \
			else \
				exit 1; \
			fi ; \
		fi ; \
	done; true

.PHONY: install-$(1)

install-$(1): modules.lst $$($(1)_dst)
	+@for r in $($(1)) "" ; do \
		if [ -n "$$$$r" -a -r "$$$$r/Makefile" ]; then \
			$(call oecho, "" ;) \
			$(call oecho, "" ;) \
			if  $(MAKE) -C $$$$r install mods_dst=$$($(1)_dst) $$(mk_params) \
				|| [ ${err_fail} != 1 ] ; then \
				:; \
			else \
				exit 1; \
			fi ; \
		fi ; \
	done; true


.PHONY: install-$(1)-doc

install-$(1)-doc: modules.lst $(doc_prefix)/$(doc_dir)$(1)
	@for r in $($(1)_basenames) "" ; do \
		if [ -n "$$$$r" -a -r $(1)/"$$$$r/Makefile" ]; then \
			if [ -f $(1)/"$$$$r"/README ]; then \
				$$(call try_err,\
					$(INSTALL_TOUCH) $(doc_prefix)/$(doc_dir)$(1)/README ); \
				$$(call try_err,\
					$(INSTALL_DOC)  $(1)/"$$$$r"/README  \
									$(doc_prefix)/$(doc_dir)$(1)/README ); \
				$$(call try_err,\
					mv -f $(doc_prefix)/$(doc_dir)$(1)/README \
							$(doc_prefix)/$(doc_dir)$(1)/README."$$$$r" ); \
			fi ; \
		fi ; \
	done; true

.PHONY: install-$(1)-man

install-$(1)-man: $(1)-man $(man_prefix)/$(man_dir)/man7
	@for r in $($(1)_basenames) "" ; do \
		if [ -n "$$$$r" -a -r $(1)/"$$$$r/Makefile" ]; then \
			if [ -f $(1)/"$$$$r"/"$$$$r".7 ]; then \
				$$(call try_err,\
				  $(INSTALL_TOUCH) $(man_prefix)/$(man_dir)/man7/"$$$$r".7 );\
				$$(call try_err,\
					$(INSTALL_MAN)  $(1)/"$$$$r"/"$$$$r".7  \
									$(man_prefix)/$(man_dir)/man7 ); \
			fi ; \
		fi ; \
	done; true


$(modules_prefix)/$(modules_dir)$(1):
		mkdir -p $$(@)

$(doc_prefix)/$(doc_dir)$(1):
		mkdir -p $$(@)


endef

# instantiate the template
$(foreach mods,$(modules_dirs),$(eval $(call MODULES_RULES_template,$(mods))))

#$(foreach mods,$(modules_dirs),$(eval  $(info DUMP: $(call MODULES_RULES_template,$(mods)))))

# build all the modules
modules-all every-module: $(modules_dirs)

$(extra_objs):
	@echo "Extra objs: $(extra_objs)" 
	@for r in $(static_modules_path) "" ; do \
		if [ -n "$$r" -a -r "$$r/Makefile"  ]; then \
			$(call oecho, "" ;) \
			$(call oecho, "Making static module $r" ;) \
			if $(MAKE) -C $$r static $(mk_params) ; then  \
				:; \
			else \
				exit 1; \
			fi ;  \
		fi ; \
	done

.PHONY: utils
utils:
	@for r in $(C_COMPILE_UTILS) "" ; do \
		if [ -n "$$r" ]; then \
			$(call oecho, "" ;) \
			$(call oecho, "" ;) \
			if  $(MAKE) -C $$r $(mk_params) || [ ${err_fail} != 1 ] ; \
			then \
				:; \
			else \
				exit 1; \
			fi ; \
		fi ; \
	done; true


dbg: sip-router
	gdb -command debug.gdb

.PHONY: makefile_vars makefile-vars
makefile_vars makefile-vars:
	echo "FLAVOUR?=$(FLAVOUR)" > Makefile.vars

.PHONY: tar
.PHONY: dist

dist: tar

tar: makefile_vars $(auto_gen_keep)
	$(TAR) -C .. \
		--exclude=$(notdir $(CURDIR))/test* \
		--exclude=$(notdir $(CURDIR))/tmp* \
		--exclude=$(notdir $(CURDIR))/debian/$(MAIN_NAME) \
		--exclude=$(notdir $(CURDIR))/debian/$(MAIN_NAME)-* \
		--exclude=$(notdir $(CURDIR))/$(MAIN_NAME)_tls* \
		--exclude=.git* \
		--exclude=CVS* \
		--exclude=.svn* \
		--exclude=.cvsignore \
		--exclude=librpath.lst \
		--exclude=libiname.lst \
		--exclude=makecfg.lst \
		--exclude=config.mak \
		--exclude=modules.lst \
		--exclude=*.[do] \
		--exclude=*.so \
		--exclude=*.il \
		--exclude=$(notdir $(CURDIR))/$(MAIN_NAME) \
		--exclude=*.gz \
		--exclude=*.bz2 \
		--exclude=*.tar \
		--exclude=*.patch \
		--exclude=.\#* \
		--exclude=*.swp \
		--exclude=*.swo \
		${tar_extra_args} \
		-cf - $(notdir $(CURDIR)) | \
			(mkdir -p tmp/_tar1; mkdir -p tmp/_tar2 ; \
			    cd tmp/_tar1; $(TAR) -xf - ) && \
			    mv tmp/_tar1/$(notdir $(CURDIR)) \
			       tmp/_tar2/"$(NAME)-$(RELEASE)" && \
			    (cd tmp/_tar2 && $(TAR) \
			                    -zcf ../../"$(tar_name)".tar.gz \
			                               "$(NAME)-$(RELEASE)" ) ; \
			    rm -rf tmp/_tar1; rm -rf tmp/_tar2

# binary dist. tar.gz
.PHONY: bin
bin:
	mkdir -p tmp/$(MAIN_NAME)/usr/local
	$(MAKE) install basedir=$(CURDIR)/tmp/$(MAIN_NAME) $(mk_params)
	$(TAR) -C tmp/$(MAIN_NAME)/ -zcf ../$(NAME)-$(RELEASE)_$(OS)_$(ARCH).tar.gz .
	rm -rf tmp/$(MAIN_NAME)

.PHONY: deb
deb:
	-@if [ -d debian ]; then \
		dpkg-buildpackage -rfakeroot -tc; \
	elif [ -d pkg/$(MAIN_NAME)/deb/debian ]; then \
		ln -s pkg/$(MAIN_NAME)/deb/debian debian; \
		dpkg-buildpackage -rfakeroot -tc; \
		rm debian; \
	else \
		ln -s pkg/debian debian; \
		dpkg-buildpackage -rfakeroot -tc; \
		rm debian; \
	fi

.PHONY: sunpkg
sunpkg:
	mkdir -p tmp/$(MAIN_NAME)
	mkdir -p tmp/$(MAIN_NAME)_sun_pkg
	$(MAKE) install basedir=$(CURDIR)/tmp/$(MAIN_NAME) \
			prefix=/usr/local $(mk_params)
	(cd pkg/solaris; \
	pkgmk -r ../../tmp/$(MAIN_NAME)/usr/local -o -d ../../tmp/$(MAIN_NAME)_sun_pkg/ -v "$(RELEASE)" ;\
	cd ../..)
	cat /dev/null > ../$(NAME)-$(RELEASE)-$(OS)-$(ARCH)-local
	pkgtrans -s tmp/$(MAIN_NAME)_sun_pkg/ ../$(NAME)-$(RELEASE)-$(OS)-$(ARCH)-local \
		IPTEL$(MAIN_NAME)
	gzip -9 ../$(NAME)-$(RELEASE)-$(OS)-$(ARCH)-local
	rm -rf tmp/$(MAIN_NAME)
	rm -rf tmp/$(MAIN_NAME)_sun_pkg


.PHONY: install
install: mk_params="compile_for_install=yes"
install: install-bin install-every-module install-cfg \
	install-doc install-man install-utils install-share

.PHONY: dbinstall
dbinstall:
	-@echo "Initializing $(MAIN_NAME) database"
	scripts/mysql/$(SCR_NAME)_mysql.sh create
	-@echo "Done"

.PHONY: README
README: $(foreach mods,$(modules_dirs),$(mods)-readme)

.PHONY: man
man: $(foreach mods,$(modules_dirs),$(mods)-man)

mk-install_dirs: $(cfg_prefix)/$(cfg_dir) $(bin_prefix)/$(bin_dir) \
			$(modules_prefix)/$(modules_dir) $(doc_prefix)/$(doc_dir) \
			$(man_prefix)/$(man_dir)/man8 $(man_prefix)/$(man_dir)/man5 \
			$(share_prefix)/$(share_dir) \
			$(foreach mods,$(modules_dirs),\
				$(modules_prefix)/$(modules_dir)$(mods) \
				$(doc_prefix)/$(doc_dir)$(mods) )

$(cfg_prefix)/$(cfg_dir): 
		mkdir -p $(cfg_prefix)/$(cfg_dir)

$(bin_prefix)/$(bin_dir):
		mkdir -p $(bin_prefix)/$(bin_dir)

$(share_prefix)/$(share_dir):
		mkdir -p $(share_prefix)/$(share_dir)

$(modules_prefix)/$(modules_dir):
		mkdir -p $(modules_prefix)/$(modules_dir)

$(doc_prefix)/$(doc_dir):
		mkdir -p $(doc_prefix)/$(doc_dir)

$(man_prefix)/$(man_dir)/man8:
		mkdir -p $(man_prefix)/$(man_dir)/man8

$(man_prefix)/$(man_dir)/man7:
		mkdir -p $(man_prefix)/$(man_dir)/man7

$(man_prefix)/$(man_dir)/man5:
		mkdir -p $(man_prefix)/$(man_dir)/man5

# note: sed with POSIX.1 regex doesn't support |, + or ? (darwin, solaris ...) 
install-cfg: $(cfg_prefix)/$(cfg_dir)
		@if [ -f etc/$(CFG_NAME).cfg ]; then \
			sed $(foreach m,$(modules_dirs),\
					-e "s#/usr/[^:]*lib/$(CFG_NAME)/$(m)\([:/\"]\)#$($(m)_target)\1#g") \
					-e "s#/usr/local/etc/$(CFG_NAME)/#$(cfg_target)#g" \
				< etc/$(CFG_NAME).cfg \
				> $(cfg_prefix)/$(cfg_dir)$(MAIN_NAME).cfg.sample; \
			chmod 644 $(cfg_prefix)/$(cfg_dir)$(MAIN_NAME).cfg.sample; \
			if [ -z "${skip_cfg_install}" -a \
					! -f $(cfg_prefix)/$(cfg_dir)$(MAIN_NAME).cfg ]; then \
				mv -f $(cfg_prefix)/$(cfg_dir)$(MAIN_NAME).cfg.sample \
					$(cfg_prefix)/$(cfg_dir)$(MAIN_NAME).cfg; \
			fi; \
		fi
		@if [ -f etc/$(CFG_NAME)-basic.cfg ]; then \
			sed $(foreach m,$(modules_dirs),\
					-e "s#/usr/[^:]*lib/$(CFG_NAME)/$(m)\([:/\"]\)#$($(m)_target)\1#g") \
					-e "s#/usr/local/etc/$(CFG_NAME)/#$(cfg_target)#g" \
				< etc/$(CFG_NAME)-basic.cfg \
				> $(cfg_prefix)/$(cfg_dir)$(MAIN_NAME)-basic.cfg.sample; \
			chmod 644 $(cfg_prefix)/$(cfg_dir)$(MAIN_NAME)-basic.cfg.sample; \
			if [ -z "${skip_cfg_install}" -a \
					! -f $(cfg_prefix)/$(cfg_dir)$(MAIN_NAME)-basic.cfg ]; then \
				mv -f $(cfg_prefix)/$(cfg_dir)$(MAIN_NAME)-basic.cfg.sample \
					$(cfg_prefix)/$(cfg_dir)$(MAIN_NAME)-basic.cfg; \
			fi; \
		fi
		@if [ -f etc/$(CFG_NAME)-oob.cfg ]; then \
			sed $(foreach m,$(modules_dirs),\
					-e "s#/usr/[^:]*lib/$(CFG_NAME)/$(m)\([:/\"]\)#$($(m)_target)\1#g") \
					-e "s#/usr/local/etc/$(CFG_NAME)/#$(cfg_target)#g" \
				< etc/$(CFG_NAME)-oob.cfg \
				> $(cfg_prefix)/$(cfg_dir)$(MAIN_NAME)-advanced.cfg.sample; \
			chmod 644 $(cfg_prefix)/$(cfg_dir)$(MAIN_NAME)-advanced.cfg.sample; \
			if [ -z "${skip_cfg_install}" -a \
					! -f $(cfg_prefix)/$(cfg_dir)$(MAIN_NAME)-advanced.cfg ]; \
			then \
				mv -f $(cfg_prefix)/$(cfg_dir)$(MAIN_NAME)-advanced.cfg.sample \
					$(cfg_prefix)/$(cfg_dir)$(MAIN_NAME)-advanced.cfg; \
			fi; \
		fi
		@# other configs
		@for r in $(C_INSTALL_CFGS) ""; do \
			if [ -n "$$r" ]; then \
				if [ -f "$$r" ]; then \
					n=`basename "$$r"` ; \
					sed $(foreach m,$(modules_dirs),\
							-e "s#/usr/[^:]*lib/$(CFG_NAME)/$(m)\([:/\"]\)#$($(m)_target)\1#g") \
						< "$$r" \
						> "$(cfg_prefix)/$(cfg_dir)$$n.sample" ; \
					chmod 644 "$(cfg_prefix)/$(cfg_dir)$$n.sample" ; \
					if [ -z "${skip_cfg_install}" -a \
							! -f "$(cfg_prefix)/$(cfg_dir)$$n" ]; \
					then \
						mv -f "$(cfg_prefix)/$(cfg_dir)$$n.sample" \
								"$(cfg_prefix)/$(cfg_dir)$$n"; \
					fi ; \
				else \
					echo "ERROR: $$r not found" ; \
					if [ ${err_fail} = 1 ] ; then \
						exit 1; \
					fi ; \
				fi ; \
			fi ; \
		done; true
		# radius dictionary
		$(INSTALL_TOUCH) $(cfg_prefix)/$(cfg_dir)/dictionary.$(CFG_NAME)
		$(INSTALL_CFG) etc/dictionary.$(CFG_NAME) $(cfg_prefix)/$(cfg_dir)

install-bin: $(bin_prefix)/$(bin_dir) $(NAME)
		$(INSTALL_TOUCH) $(bin_prefix)/$(bin_dir)/$(NAME)
		$(INSTALL_BIN) $(NAME) $(bin_prefix)/$(bin_dir)


install-share: $(share_prefix)/$(share_dir)
	@for r in $(C_INSTALL_SHARE) "" ; do \
		if [ -n "$$r" ]; then \
			if [ -f "$$r" ]; then \
				$(call try_err, $(INSTALL_TOUCH) \
						$(share_prefix)/$(share_dir)/`basename "$$r"` ); \
				$(call try_err, \
					$(INSTALL_SHARE)  "$$r"  $(share_prefix)/$(share_dir) );\
			else \
				echo "ERROR: $$r not found" ; \
				if [ ${err_fail} = 1 ] ; then \
					exit 1; \
				fi ; \
			fi ;\
		fi ; \
	done; true


install-every-module: $(foreach mods,$(modules_dirs),install-$(mods))

install-every-module-doc: $(foreach mods,$(modules_dirs),install-$(mods)-doc)

install-every-module-man: $(foreach mods,$(modules_dirs),install-$(mods)-man)

install-utils: utils $(bin_prefix)/$(bin_dir)
	@for r in $(C_INSTALL_BIN) "" ; do \
		if [ -n "$$r" ]; then \
			if [ -f "$$r" ]; then \
				$(call try_err, $(INSTALL_TOUCH) \
						$(bin_prefix)/$(bin_dir)/`basename "$$r"` ); \
				$(call try_err,\
					$(INSTALL_BIN)  "$$r"  $(bin_prefix)/$(bin_dir) ); \
			else \
				echo "ERROR: $$r not compiled" ; \
				if [ ${err_fail} = 1 ] ; then \
					exit 1; \
				fi ; \
			fi ;\
		fi ; \
	done; true
	@for r in $(C_INSTALL_SCRIPTS) "" ; do \
		if [ -n "$$r" ]; then \
			if [ -f "$$r" ]; then \
				$(call try_err, $(INSTALL_TOUCH) \
						$(bin_prefix)/$(bin_dir)/`basename "$$r"` ); \
				$(call try_err,\
					$(INSTALL_SCRIPT)  "$$r"  $(bin_prefix)/$(bin_dir) ); \
			else \
				echo "ERROR: $$r not compiled" ; \
				if [ ${err_fail} = 1 ] ; then \
					exit 1; \
				fi ; \
			fi ;\
		fi ; \
	done; true
	@for ut in $(C_INSTALL_UTILS) "" ; do \
		if [ -n "$$ut" ]; then \
			if [ -d "$$ut" ]; then \
				$(call try_err, $(MAKE) -C "$${ut}" install-if-newer ) ;\
			fi ;\
		fi ; \
	done; true


install-modules-all: install-every-module install-every-module-doc


install-doc: $(doc_prefix)/$(doc_dir) install-every-module-doc
	$(INSTALL_TOUCH) $(doc_prefix)/$(doc_dir)/INSTALL 
	$(INSTALL_DOC) INSTALL $(doc_prefix)/$(doc_dir)
	$(INSTALL_TOUCH) $(doc_prefix)/$(doc_dir)/README-MODULES 
	$(INSTALL_DOC) README-MODULES $(doc_prefix)/$(doc_dir)
	$(INSTALL_TOUCH) $(doc_prefix)/$(doc_dir)/AUTHORS 
	$(INSTALL_DOC) AUTHORS $(doc_prefix)/$(doc_dir)
	$(INSTALL_TOUCH) $(doc_prefix)/$(doc_dir)/NEWS
	$(INSTALL_DOC) NEWS $(doc_prefix)/$(doc_dir)
	$(INSTALL_TOUCH) $(doc_prefix)/$(doc_dir)/README 
	$(INSTALL_DOC) README $(doc_prefix)/$(doc_dir)


install-sr-man: $(man_prefix)/$(man_dir)/man8 $(man_prefix)/$(man_dir)/man5
		@sed -e "s#/etc/$(CFG_NAME)/$(CFG_NAME)\.cfg#$(cfg_target)$(MAIN_NAME).cfg#g" \
			-e "s#/usr/sbin/#$(bin_target)#g" \
			$(foreach m,$(modules_dirs),\
				-e "s#/usr/lib/$(CFG_NAME)/$(m)\([^_]\)#$($(m)_target)\1#g") \
			-e "s#/usr/share/doc/$(CFG_NAME)/#$(doc_target)#g" \
			-e "s#$(SRC_NAME)#$(MAIN_NAME)#g" \
			< $(SRC_NAME).8 >  \
							$(man_prefix)/$(man_dir)/man8/$(MAIN_NAME).8
		@chmod 644  $(man_prefix)/$(man_dir)/man8/$(MAIN_NAME).8
		@sed -e "s#/etc/$(CFG_NAME)/$(CFG_NAME)\.cfg#$(cfg_target)$(MAIN_NAME).cfg#g" \
			-e "s#/usr/sbin/#$(bin_target)#g" \
			$(foreach m,$(modules_dirs),\
				-e "s#/usr/lib/$(CFG_NAME)/$(m)\([^_]\)#$($(m)_target)\1#g") \
			-e "s#/usr/share/doc/$(CFG_NAME)/#$(doc_target)#g" \
			-e "s#$(SRC_NAME)#$(MAIN_NAME)#g" \
			< $(SRC_NAME).cfg.5 >  \
			$(man_prefix)/$(man_dir)/man5/$(MAIN_NAME).cfg.5
		@chmod 644  $(man_prefix)/$(man_dir)/man5/$(MAIN_NAME).cfg.5

install-man:  install-sr-man install-every-module-man



# libs cleaning targets
.PHONY: clean-libs
clean-libs:
			$(MAKE) -C lib clean

.PHONY: proper-libs realclean-libs distclean-libs maintainer-clean-libs
proper-libs realclean-libs distclean-libs maintainer-clean-libs:
			$(MAKE) -C lib $(patsubst %-libs,%,$@)

# utils cleaning targets

.PHONY: clean-utils
clean-utils:
	@for r in $(C_COMPILE_UTILS) $(C_INSTALL_UTILS) "" ; do \
		if [ -d "$$r" ]; then \
			 $(MAKE) -C "$$r" clean ; \
		fi ; \
	done

.PHONY: proper-utils
.PHONY: distclean-utils
.PHONY: realclean-utils
.PHONY: maintainer-clean-utils
proper-utils realclean-utils distclean-utils maintainer-clean-utils: \
 clean_target=$(patsubst %-utils,%,$@)
proper-utils realclean-utils distclean-utils maintainer-clean-utils:
	@for r in $(C_COMPILE_UTILS) $(C_INSTALL_UTILS) "" ; do \
		if [ -d "$$r" ]; then \
			 $(MAKE) -C "$$r" $(clean_target); \
		fi ; \
	done

# clean extra binary names (common "flavour" names)
clean: clean-extra-names
# clean modules on make clean
clean: clean-modules
# clean utils on make clean
clean: clean-utils
# cleaning in libs always when cleaning sip-router
clean: clean-libs

.PHONY: clean-extra-names
clean-extra-names:
	@rm -f $(filter-out $(MAIN_NAME), sip-router ser kamailio)

# proper/distclean a.s.o modules, utils and libs too

proper: clean-extra-names proper-modules proper-utils proper-libs
distclean: distclean-modules distclean-utils distclean-libs
realclean: realclean-modules realclean-utils realclean-libs
maintainer-clean: maintainer-clean-modules maintainer-clean-utils \
 maintainer-clean-libs

#try to clean everything (including all the modules, even ones that are not
# configured/compiled normally
.PHONY: clean-all
clean-all: cmodules=$(all_modules_lst)
clean-all: clean
maintainer-clean: modules=$(modules_all)

# on make proper clean also the build config (w/o module list)
proper realclean distclean maintainer-clean: clean_cfg

# on maintainer clean, remove also the configured module list
maintainer-clean: clean_modules_cfg clean_makefile_vars

.PHONY: proper-all realclean-all distclean-all
proper-all realclean-all distclean-all: cmodules=$(all_modules_lst)
proper-all realclean-all distclean-all: proper


.PHONY: clean_cfg clean-cfg
clean_cfg clean-cfg:
	rm -f config.mak

.PHONY: clean_modules_cfg clean-modules-cfg
clean_modules_cfg clean-modules-cfg:
	rm -f modules.lst

.PHONY: clean_makefile_vars clean-makefile-vars
	rm -f Makefile.vars

.PHONY: dbschema
dbschema:
	-@echo "Build database schemas"
	$(MAKE) -C lib/srdb1/schema
	-@echo "Done"
