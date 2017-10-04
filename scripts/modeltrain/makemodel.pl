#!/usr/bin/perl
$do_standardise = 0; # TODO: implement speaker standardisation...
$do_select = 0;
$regression = 0;  # 0: classification model , 1: regression model

#--------------
use File::Basename;
require "arff-functions.pl";

if ($#ARGV < 0) {
  print "\nUsage: perl makemodel.pl <corpus_path ¦ arff file (must end in .arff)>  [SMILExtract config for corpus mode]\n\n";
  exit -1;
}

$xtract = "../../SMILExtract";

mkdir ("built_models");
mkdir ("work");
$corp = $ARGV[0];  # full path to corpus OR direct arff file...
if ($corp =~ /\.arff$/i) {
  $mode="arff";
  unless (-f "$corp") {
    print "ERROR '$corp' is not an ARFF file or does not exist!\n";
    exit;
  }
  $cname=basename($corp);
  $cname =~s/\.arff$//i;
  $arff = $corp;
  $modelpath = "built_models/$cname";
  $workpath = "work/$cname";
} else {
  $mode = "corp";
  unless (-d "$corp") {
    print "ERROR '$corp' is not a corpus directory or does not exist!\n";
    exit;
  }
  $corp =~ /\/([^\/]+)_FloEmoStdCls/;
  $cname = $1;
  $conf = $ARGV[1];
  unless ($conf) { $conf = "is09s.conf"; }
  $cb=$conf; $cb=~s/\.conf$//;
  $modelpath = "built_models/$cname";
  $workpath = "work/$cname";
  $arff = "$workpath/$cb.arff";
}

  mkdir("$modelpath");
  mkdir("$workpath");


#extract features
if ($mode eq "corp") {
  print "-- Corpus mode --\n  Running feature extraction on corpus '$corp' ...\n";
  system("perl stddirectory_smileextract.pl \"$corp\" \"$conf\" \"$arff\"");
} else {
  print "-- Arff mode --\n  Copying '$arff' to work directory ...\n";
  $arffb = basename($arff);
#  print("cp $arff $workpath/$arffb");
  system("cp $arff $workpath/$arffb");
  $arff = "$workpath/$arffb";
}

# ? standardise features
if ($do_standardise) {
 print "NOTE: standardsise not implemented yet, svm-scale will do the job during building of model\n";
}

# ? select features
$lsvm=$arff; $lsvm=~s/\.arff$/.lsvm/i;
if ($do_select) {
 print "Selecting features (CFS)...\n";
 $fself = $arff;
 $fself=~s/\.arff/.fselection/i;
 system("perl fsel.pl $arff");
 $arff = "$arff.fsel.arff";
} else {
  $fself="";
  print "Converting arff to libsvm feature file (lsvm) ...\n";
  # convert to lsvm
  my $hr = &load_arff($arff);
  my $numattr = $#{$hr->{"attributes"}};
  if ($hr->{"attributes"}[0]{"name"} =~ /^name$/) {
    $hr->{"attributes"}[0]{"selected"} = 0;  # remove filename
  }
  if ($hr->{"attributes"}[0]{"name"} =~ /^filename$/) {
    $hr->{"attributes"}[0]{"selected"} = 0;  # remove filename
  }
  if ($hr->{"attributes"}[1]{"name"} =~ /^timestamp$/) {
    $hr->{"attributes"}[1]{"selected"} = 0;  # remove filename
  }
  if ($hr->{"attributes"}[1]{"name"} =~ /^frameIndex$/) {
    $hr->{"attributes"}[1]{"selected"} = 0;  # remove filename
    if ($hr->{"attributes"}[2]{"name"} =~ /^frameTime$/) {
      $hr->{"attributes"}[2]{"selected"} = 0;  # remove filename
    }
  }
  if ($hr->{"attributes"}[1]{"name"} =~ /^frameTime$/) {
    $hr->{"attributes"}[1]{"selected"} = 0;  # remove filename
  }
   #$hr->{"attributes"}[$numattr-1]{"selected"} = 0; # remove continuous label
  &save_arff_AttrSelected($arff,$hr);
  system("perl arffToLsvm.pl $arff $lsvm");
}

# train model
print "Training libsvm model ...\n";
system("perl buildmodel.pl $lsvm");

$discfile = $lsvm; $discfile=~s/\.lsvm$/.disc/;
open(FILE,"<$discfile");
$disc=<FILE>; $disc=~s/\r?\n$//;
close(FILE);

# copy final files to modelpath
$scale = $lsvm; $scale=~s/\.lsvm$/.scale/;
$model = $lsvm; $model=~s/\.lsvm$/.model/;
$cls = $lsvm; $cls=~s/\.lsvm$/.classes/;
if ($cb) { $cb = "$cb."; }

if ($do_select) { $sel="cfssel."; }
else { $sel="allft."; }

system("cp $scale $modelpath/$cb$sel"."scale");
system("cp $model $modelpath/$cb$sel"."model");
if ($disc) {
  system("cp $cls $modelpath/$cb$sel"."classes");
}
if ($fself) {
  system("cp $fself $modelpath/$cb"."fselection");
}

