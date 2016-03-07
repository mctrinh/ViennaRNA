#!/usr/bin/perl

use strict;
use warnings;
use Getopt::Long qw( :config posix_default bundling no_ignore_case );
use Pod::Usage;
use RNA::Design;

# TODO: 
#   *) Make penalty for bprobs accessible and distribute non-specialized values
#       => + update manpage

# ********************** #
# Initialize RNA::Design #
# ...................... #
my $ViennaDesign = RNA::Design->new();

# ******************* #
# Get Input Arguments #
# ................... #

my @structs;
my $constr;
my $length = 0;
my $cutpnt = -1;

# These are also the defaults in RNA::Design.pm
my $optfun = 'eos(1)+eos(2)-2*efe() + 0.3*(eos(1)-eos(2)+0.00)**2';
my $scores = ({AAAAA => 5, CCCCC => 5, GGGGG => 5, UUUUU => 5});
my $bprobs = ({A => 0.25, C => 0.25, G => 0.25, U => 0.25});

my ($n, $m) = (1, 1e10);
my $startseq =undef;
my $ParamFile=undef;
my $verbose = 0;
my $jango = 0;

GetOptions(
  # ViennaDesign parameters
  "o|optfun=s"  => sub{$optfun = check_input_costfunction($_[1])},
  "a|avoid=s"   => sub{$scores = check_input_avoid($_[1])},
  "p|bprobs=s"  => sub{$bprobs = check_input_base_probs($_[1])},
  "n|number=i"  => \$n, # number of independent runs
  "m|maxiter=i" => \$m, # maximal number of steps
  "s|start=s"   => \$startseq,

  # ViennaRNA energy parameters
  "P|params=s"  => \$ParamFile,
  "4|tetloops"  => sub {$RNA::tetra_loop=0},
  "d|dangle=i"  => sub {$RNA::dangles=$_[1]},
  "noGU"        => \$RNA::noGU,
  "noCloseGU"   => \$RNA::no_closingGU,

  # other
  "v|verbose=i" => \$verbose,
  "j|jango=i"   => \$jango,

  "h|help"      => sub{pod2usage(1)},
  "man"         => sub{pod2usage(-exitstatus => 1, -verbose => 2)},
) or pod2usage(1);

read_input(\@structs, \$constr, \$length, \$cutpnt);

--$length if $cutpnt != -1;
$constr = 'N' x $length unless $constr;

# ******************* #
# Set Input Arguments #
# ................... #
$ViennaDesign->set_cut_point($cutpnt);
$ViennaDesign->set_structures(@structs);
$ViennaDesign->set_constraint($constr);
$ViennaDesign->set_optfunc($optfun);
$ViennaDesign->set_base_probs($bprobs);
$ViennaDesign->set_score_motifs($scores);
RNA::read_parameter_file($ParamFile) if ($ParamFile);
#alternative: $ViennaDesign->set_parameter_file($ParamFile) if $ParamFile;

if ($verbose) {
  print "Option-check:\n";
  print "Score: $_ = $$scores{$_}\n" foreach keys %$scores;
  print "Base-prob $_ = $$bprobs{$_}\n" foreach keys %$bprobs;
  print "Objective Function: ".$ViennaDesign->get_optfunc."\n";
  $ViennaDesign->set_verbosity($verbose);
}

$ViennaDesign->find_dependency_paths;
$ViennaDesign->explore_sequence_space;
#$ViennaDesign->eval_sequence($ViennaDesign->find_a_sequence);

# **************** #
# Main Design Loop #
# ................ #


my %final;
my ($seq, $cost, $count);
for (1 .. $n) {

  # Jango-Method: 
  if ($jango) {
    warn "Jango with $jango presamples\n";
    die "Conflicting Options (--jango & --start)" if $startseq;
    $startseq = $ViennaDesign->find_a_sequence;
    for (my $c=0; $c<$jango; ++$c) {
      my $tmpseq = $ViennaDesign->find_a_sequence;
      if ($ViennaDesign->eval_sequence($tmpseq) < $ViennaDesign->eval_sequence($startseq)) {
        $startseq = $tmpseq;
      }
    }
  }

  $seq = ($startseq) ? $startseq : $ViennaDesign->find_a_sequence;
  $seq = $ViennaDesign->optimize_sequence($seq, $m);

  if (exists $final{$seq}) {
    $cost = $final{$seq}[0];
    $final{$seq}[1]++;
  } else {
    $cost = $ViennaDesign->eval_sequence($seq);
    $final{$seq}[0] = $cost;
    $final{$seq}[1]++;
  }
}

# ************* #
# Print Results #
# ............. #

my $id=1;
foreach $seq (sort {$final{$a}[0]<=>$final{$b}[0]} keys %final) {
  my $cseq = $seq;
  substr $cseq, $cutpnt-1,0,'&' if ($ViennaDesign->get_cut_point != -1);
  printf "%3d %s %6.2f %d\n", $id++, $cseq, $final{$seq}[0], $final{$seq}[1];
}

#############################

sub read_input {
  my ($str,$con,$len,$cut) = @_;

  while (<>) {
    chomp;
    next if $_ eq '';

    # Check if length is consistent
    # TODO treat sequeces with '$' different to allow for cotranscr
    $$len = length $_ unless $$len;
    if (length $_ != $$len) {
      die "All structures/constraints must have the same length!";
    }

    # Set cut-point
    my $tmp = index $_, '&';
    if ($tmp != -1 || $$cut != -1) { # it is a cofold-design!
      ++$tmp;
      if ($$cut == -1) {
        $$cut = $tmp;
      } elsif ($tmp != $$cut) {
        die "cut_points ('&') may not differ in input";
      }
      s/&//; 
      die "only one cut_point ('&') allowed" if s/&//;
    }

    if (m/[\(\)\.x\&]/g) { # is a structure constraint!
      if (m/[^\(\)\.\&x]/g) { # only valid characters!
        die "$_ contains forbidden character: $&";
      } 
      push @$str, $_;
    } elsif (m/[ACUGTURYSMWKVHDBN]/) { # is a sequence constraint!
      if (m/[^ACUGTURYSMWKVHDBN&]/g) { # only valid characters!
        die "Constraint contains a non-iupack caracter: $&";
      }
      die "Only one constraint allowed!" if ($$con);
      $$con = $_;
    }
  }
  return 1;
}

sub check_input_costfunction {
  # TODO: need to check this in a better way!
  # TODO: check for large integers (especially in **2)
  my $optfun = shift;

  # TODO: remove these lines once the web is up-to-date
  $optfun =~ s/gfe/efe/g;
  $optfun =~ s/pfc/efe/g;

  my @allowed = qw( + - / * . );
  foreach my $f1 (@allowed) {
    foreach my $f2 (@allowed) {
      my $forbidden = $f1.$f2;
      if ($forbidden eq '**') {
        die "'***' not allowed in objective function" if (index($optfun, '***') != -1);
      } else {
        die "\'$forbidden\' not allowed in objective function" if (index($optfun, $forbidden) != -1);
      }
    }
  }
  my $nakedfun = $optfun;
  foreach my $fb ('eos', 'efe', 'prob', '_circ', 'barr') {
    $nakedfun =~ s/$fb//g;
  }
  die "cannot interpret \'$&\' in objective function" if ($nakedfun =~ m/[^\-\+\/\(\)\.\*\d\,\s]+/g);
  die "unbalanced brackets in optimization function" if scalar($nakedfun =~ tr/\(//) != scalar($nakedfun =~ tr/\)//);

  return $optfun;
}

sub check_input_avoid {
  my $input = shift;
  my %motifs;

  foreach my $a (split ',', $input) {
    my @tmp = (split ':', $a);
    $motifs{$tmp[0]}=$tmp[1];
  }

  foreach my $a (keys %motifs) {
    $a = uc($a);
    die "String $a in 'avoid' list may only contain A,C,U/T or G" if $a =~ m/[^ACUGT]/g;
    die "Scoring the motif $a with 0 does not make sense!" if $motifs{$a} == 0;
  }

  return (%motifs) ? \%motifs: undef;
}

sub check_input_base_probs {
  my $input = shift;
  my %bprobs;

  foreach my $a (split ',', $input) {
    my @tmp = (split ':', $a);
    $bprobs{$tmp[0]}=$tmp[1];
  }

  if (%bprobs) {
    my $tot=0;
    foreach my $b (sort keys %bprobs) {
      die "cannot interpret base: $_ in baseprobabilties" if $b =~ m/[^ACUG]/g || length $b > 1;
      die "bad probability for $b: $bprobs{$b}!" unless $bprobs{$b} =~ m/^\d*\.?\d+$/g;
      $tot+=$bprobs{$b};
    }
    die "Base probabilities must sum up to 1. Currently: $tot\n" if ($tot < 0.98 || $tot > 1.02);
  }
  return (%bprobs) ? \%bprobs : undef;
}

__END__

=head1 NAME

RNAdesign.pl - Flexible design of multistable RNA molecules

=head1 SYNOPSIS

RNAdesign.pl [-o, --options] < constraints.in

=head1 DESCRIPTION

Design an RNA (or DNA) via adaptive walks in the sequence landscape. The input
file specifies both structure and sequence constraints. Multiple secondary
structure constraints followed by an (optional) sequence constraint in I<IUPAC>
code (i.e. A, C, G, U/T, N, R, Y, S, M, W, K, V, H, D, B) are strictly enforced
during the design process. However, B<RNAdesign.pl> avoids difficulties of
multstable designs where a single nucleotide has more than two dependencies. In
that case, base-pair constraints are not strictly enforced, but still evaluated
in the objective function. A warning will be printed to *STDERR*.

Secondary structures must be specified with a well-balanced dot-bracket terms.
They may contain the following special characters: B<'&'> connects two sequences
to design a pair of interacting RNAs; B<'x'> when a structure is used in the
objective function to compute the accessibility of nucleotides (i. e. the
probability of being unpaired).

  Input example:
  ((((....((((...))))...))))
  ((((....))))...((((...))))
  ............xxx...........
  NNNNGNRANNNNNNNNNNNNNNAUGN

=head1 PREREQUISTES

This script is part of the B<ViennaRNA package>. It requires the B<RNA::Design> perl
library, which has been introduced in B<ViennaRNA package-v2.2>.

=head1 WEB

A web interface to the B<RNA::Design> library is available at

  http://rna.tbi.univie.ac.at/rnadesign

=head1 CITE

For Details on the Algorithms see

  Badelt S., "Control of RNA function by conformational design", PhD Thesis (2016)

  Flamm C., et al. "Design of Multi-Stable RNA Molecules", RNA 7:254-265 (2001)

=head1 OPTIONS

=over 4

=item B<-o, --optfun> <string>

The objective function can be customized using a simple interface to the
functions of the B<ViennaRNA package>. Every input secondary structure *can*
serve as full target conformation or structure constraint. The objective
function can include terms to compute the free energy of a target structure,
the (constrained) ensemble free energy, the (conditional) probabilities of
secondary structure elements, the accessibility of subsequences and the
direct-path barriers between two structures. All of these terms exist for
linear, circular, and cofolded molecules, as well as for custom specified
temperatures. Indices B<i, j> correspond to the secondary structures specified
in the input file, B<t> is optional to specify the temperature in Celsius. By
default, computations use the standard temperature of 37C.

  eos(i,t): Energy of structure i at temperature t. [circular: eos_circ(i,t)]

  efe(i,t): Free energy of a constrained secondary structure ensemble. Omitting
  i, or specifying i=0 computes the unconstraint enemble. [circular: efe_circ(i,t)]

  prob(i,j,t): Probability of structure i given structure j. The probability is 
  computed form the partition functions Pr(i|j)=Z_i/Z_j. Hence, the constraint j 
  should include the constraint of i. [circular: prob_circ(i,j,t)]

  acc(i,t): Accessibility of an RNA/DNA motif specified with 'x' in the
  structure input. [circular: acc_circ(i,t)]

  barr(i,j,t): direct path energy barrier from i to j computed using findpath.
  [circular: not implemented]

By default, two independent penalties are added to the objective function, see
options B<--avoid> and B<--bprobs>. 

Default: I<'eos(1)+eos(2)-2*efe()+0.3*(eos(1)-eos(2)+0.00)**2'>

=item B<-a, --avoid> <string>

A set of sequence motifs that recieve an extra penalty. Whenever one of these
motifs is found in a sequence, its penalty is added to the score of the
objective function. Specify pairs of B<motif:penalty> as a comma-separated
string.

Default: I<AAAAA:5,CCCCC:5,GGGGG:5,UUUUU:5>

=item B<-p, --bprobs> <string>

Set a target distribution of nucleotides in the designed sequence. Whenever the
observed nucleotide content differs from target values, a penalty is added to the
objective function (see B<--optfun>). Let B<p> be a vector of specified base
probabilities and B<q> a vector of observed nucleotide percentages, then the
similarity of these vectors is computed as B<s=sum_n(sqrt(p_n*q_n))> where B<n>
is the index for B<A,C,G,U>. The penalty is calculated B<(1-s)*100>. Specify
base probabilities as a comma-separated string of <base>:<prob> tuples.

Default: I<A:0.25,C:0.25,G:0.25,U:0.25>

=item B<-n, --number> <int>

Specify the number of independent sequence designs. Default: B<1>

=item B<-m, --maxiter> <int>

Maximum number of trial/error improvements during adaptive walk. The default is
B<1e10>, however, this threshold is only important for large sequence designs,
as the value is caculated automatically form the number of possible sequence
mutations. 

=item B<-s, --start> <string>

Specify a starting sequence for adaptive optimization.

=item B<-P, --params> <path-to-file>

Specify a Parameter file other than B<rna_turner2004>. Alternative DNA and RNA
parameter files are distributed with the B<ViennaRNA package>.

=item B<-4, --tetloops> <flag>

Turn off parameters for particular tetra-loop hairpin motifs.

=item B<-d, --dangle> <int [0,1,2,3]>

ViennaRNA dangling energy model. Default: 2

=item B<--noGU> <flag>

Turn off energies for GU basepairs. 

=item B<--noCloseGU> <flag>

Turn off energies for GU basepairs at the end of helices.

=item B<-v, --verbose> <int>

Use *stderr* to report every sequence accepted during the adaptive walk

=item B<-h, --help>

Print short help

=item B<--man>

Prints the manual page and exits

=back

=head1 AUTHOR

Stefan Badelt E<lt>stef@tbi.univie.ac.atE<gt>

=cut

