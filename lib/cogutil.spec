#
# spec file for RPM package cogutil
#
# Copyright (C) 2008 by Singularity Institute for Artificial Intelligence
# All Rights Reserved
#
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#
# Please submit bugfixes or comments to gama@vettalabs.com 
#

Name:           cogutil
Version:        2.0.3
Release:        0
Summary:        A collection of basic utilities for opencog

Group:          Development/Libraries
License:        AGPLv3
URL:            http://opencog.org
Source0:        file:///home/gama/rpmbuild/SOURCES/cogutil-%{version}.tar.bz2
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)


BuildRequires:  cmake >= 2.8.8, make >= 3, gcc-c++ >= 5.0, boost-devel >= 1.46, glibc-devel >= 2.3

%description
The Open Cognition Framework (OpenCog) utilities provide a collection
of useful utilities that are used throughout the OpenCog code base.

%prep
%setup -q


%build
cmake -DCMAKE_INSTALL_PREFIX:STRING=/usr -DCONFDIR=/etc -DCMAKE_BUILD_TYPE:STRING=RelWithDebInfo .
make %{?_smp_mflags}


# disable fedora's automatic debug package generation
%define debug_package %{nil}

%install
rm -rf %buildroot
mkdir %buildroot
make install DESTDIR=%buildroot


%package -n cogutil
Summary: The OpenCog utilities
Group: Development/Libraries
AutoReqProv: 0
Requires: libstdc++ >= 5.0, glibc >= 2.3
%description -n libcogutil
The libcogutil library provides assorted utilities
%files -n libcogutil
%defattr(-,root,root,-)
%doc LICENSE
/%{_libdir}/opencog/libcogutil.so


%package -n libcogutil-devel
Summary: Header files for the libcogutil library
Group: Development/Libraries
AutoReqProv: 0
Requires: libcogutil >= %{version}, gcc-c++ >= 5.0, boost-devel >= 1.46, glibc-devel >= 2.3
%description -n libcogutil-devel
The libcogutil-devel package contains the header files needed to develop
programs that use the libcogutil library (which is part of the Open Cognition
Framework).
%files -n libcogutil-devel
%defattr(-,root,root,-)
%doc LICENSE
/%{_includedir}/opencog/util/*


%clean
rm -rf %buildroot


%changelog
* Wed Oct 01 2008 Gustavo Machado Campagnani Gama <gama@vettalabs.com> 0.1.4-0
- Initial RPM release
* Wed Sep 10 2012 Linas Vepstas <linasvepstas@gmail.com>
- Remove obviously obsolete packages, dependencies.
* Fri Dec 14 2018 Linas Vepstas <linasvepstas@gmail.com>
- Remove obviously wrong stuff.
