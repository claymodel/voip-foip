use ExtUtils::MakeMaker;

WriteMakefile (
        NAME		=> 'callweaver-perl',
        VERSION_FROM	=> 'lib/CallWeaver.pm',
        AUTHOR		=> 'James Golovich <james@gnuinter.net>',
        ABSTRACT	=> 'callweaver.org Perl Interface',
	PMLIBDIRS	=> [ 'lib' ],
	PREREQ_PM	=> { 'IO::Socket' => 0, 
			     'Digest::MD5' => 0 },
	'dist'		=> {
				COMPRESS => 'gzip -9f', SUFFIX => 'gz',
				DIST_DEFAULT => 'tardist'
			   },
);

