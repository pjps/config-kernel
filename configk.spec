%global debug_package %{nil}

Name:           config-kernel
Version:        0.2
Release:        1%{?dist}
Summary:        An easy way to edit kernel configuration files and templates

License:        GPLv2+
Group:		 Application
URL:            https://github.com/pjps/config-kernel
Source0:        %{URL}/archive/refs/tags/%{name}-%{version}.tar.gz

BuildRequires:  bison flex gcc make bison-devel
Requires:       bison flex gcc make bison-devel

%if 0%{?fedora}
BuildRequires:  libfl-devel
Requires:       libfl-devel
%else
BuildRequires:  libfl-static
Requires:       libfl-static
%endif

%description
config-krenel tool helps to edit kernel configuration files and templates.
User can query, enable, disable or toggle CONFIG options via command line
switch or an $EDITOR program, without worrying about option dependencies.

%prep
%autosetup


%build
make %{?_smp_mlags}


%install
mkdir -p %{buildroot}/%{_bindir}
cp configk %{buildroot}/%{_bindir}/


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr (-,root,root,-)
#%license add-license-file-here
%doc README.md COPYING
%{_bindir}/configk


%changelog
* Wed Jan 10 2024 pjp <pjp@fedoraproject.org> - 0.2-1
- Initial config-kernel package
