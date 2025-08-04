eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

use strict;
use warnings;

use PerlDDS::Run_Test;

my $test = new PerlDDS::TestFramework();


sub start_mc {
  my $name = shift();
  my $priority = shift();
  my $time = shift();

  $test->process($name, 'basic-mc', "$name $priority $time");
  $test->start_process($name);

  return $name, time() + $time;
}

$test->process('dev', 'basic-dev');
$test->start_process('dev');

my $extra = 3;
my $lost_controller = 9 + $extra; # 3s for missed, 6s for lost

sub expect_new_mc {
  my $name = shift();
  my $max = shift();

  return $test->wait_for('dev', "new controller $name", max_wait => $max);
}

sub wait_until {
  my $when = shift();

  my $til = $when - time();
  if ($til > 0) {
    sleep($til);
  }
}

sub expect_stop_mc {
  my $name = shift();
  my $ends_at = shift();

  wait_until($ends_at);
  kill('INT', PerlDDS::TestFramework::_getpid($test->{processes}->{process}->{$name}->{process}));
  $test->stop_process($ends_at - time() + $extra, $name);
}

sub expect_lost_mc {
  my $name = shift();
  my $max = shift();

  return $test->wait_for('dev', "lost controller $name", max_wait => $max);
}

my ($mc1, $mc1_ends_at) = start_mc('mc1', 0, 20);
expect_new_mc($mc1, 10);

sleep(3);
my ($mc2, $mc2_ends_at) = start_mc('mc2', 10, 30);
my ($mc3, $mc3_ends_at) = start_mc('mc3', 0, 30);
my ($mc4, $mc4_ends_at) = start_mc('mc4', 10, 30);

expect_stop_mc($mc1, $mc1_ends_at);
expect_lost_mc($mc1, $lost_controller);
expect_new_mc($mc3, 5);
expect_stop_mc($mc2, $mc2_ends_at);
expect_stop_mc($mc3, $mc3_ends_at);
expect_stop_mc($mc4, $mc4_ends_at);

$test->kill_process(2, 'dev');
exit $test->finish(2);
