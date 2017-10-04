#!/usr/bin/perl

$input_lsvm = $ARGV[0];

$scale = $input_lsvm; $scale=~s/\.lsvm$/.scale/; 
$scaled_lsvm = $input_lsvm; $scaled_lsvm =~ s/\.lsvm/.scaled.lsvm/;
$model = $input_lsvm; $model=~s/\.lsvm$/.model/;
 
# scale features, build model

system("libsvm-small/svm-scale -s $scale $input_lsvm > $scaled_lsvm");

$discfile = $input_lsvm; $discfile=~s/\.lsvm$/.disc/;
open(FILE,"<$discfile");
$disc=<FILE>; $disc=~s/\r?\n$//;
close(FILE);

if ($disc) { # classification if disc==1

print "  building an SVM CLASSFICATION model...\n";
#classification:
system("libsvm-small/svm-train -b 1 -s 0 -t 1 -d 1 -c 0.3 $scaled_lsvm $model");

} else { # regression otherwise:

print "  building an SVR REGRESSION model...\n";
#regression
system("svm-train -s 3 -t 1 -d 1 -c 0.4 $scaled_lsvm $model");


}

