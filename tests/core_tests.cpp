#include "../Source/ScraperCore.h"
#include <cassert>

int main()
{
    scraper::Sequencer s;
    s.regenerate (123, 1.0f);
    s.reset();
    auto first = s.advanceTo (0);
    auto duplicate = s.advanceTo (0);
    assert (! duplicate.fired);
    assert (first.step.slice < 16);
    s.reset();
    assert (! s.advanceTo (0, 0.0f).fired);
    s.mutate (99, 0.5f);
    assert (scraper::semitonesToRatio (12.0f) > 1.999 && scraper::semitonesToRatio (12.0f) < 2.001);
}
