#!/usr/bin/perl


# openSMILE extraction script 
# usage: $0 <corpus base path> <opensmile config file> <output arff>

# path to SMILExtract binary (incl trailing /)
$sepath = "../../";


#-------------------------------------------------------------------------


$corp = $ARGV[0];

$wavpath = "$corp";

$pwd=`pwd`;
$conf = $ARGV[1];
unless ($conf) { $conf="is09s.conf"; }


$confb=$conf; $confb=~s/\.conf//i;
$arff = $ARGV[2];

use File::Basename;
#$bn=basename($ft);
#$arff="functionals_$bn.arff";

#-----
$wavpath =~ s/ /\\ /g;
@cl = glob("$wavpath"."/*");
$labs="";
my @cls;
foreach $c (@cl) {
 if (-d "$c") {
   $c = basename($c);
   push(@cls,$c);
   print "label: ".$c."\n";
   $labs .= "$c,";
 }
}
chop($labs);

#!!!!!!!!
unlink ("$arff");

foreach $c (@cls) {

print $c."\n";
@wavs = glob("$wavpath/$c/*.wav");
foreach $w (@wavs) {
  print $w."... ";
  $bn=basename($w);
  $bn=~ s/\.wav$//i;
 
  # print("mplayer -ao pcm:waveheader:fast:file=\"$wave\" -vc null -vo null \"$w\"");
  #exit;

  $ret = system("$sepath./SMILExtract -C $sepath./config/$conf -I \"$w\" -O \"$arff\" -instname \"$bn\" -classes \"\{$labs\}\" -classlabel \"$c\" -corpus \"$corp\" 2>/dev/null"); #-o \"$work/features/lld/Train/$w.htk\"
  if ($ret != 0) { print "ERROR!\n\n"; exit; }
  else { print "OK.\n"; }
}


}

