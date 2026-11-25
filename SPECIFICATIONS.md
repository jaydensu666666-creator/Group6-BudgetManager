# MVP - Specifications

What the system does to meet each user requirement in
[REQUIREMENTS.md](REQUIREMENTS.md). Money is in US dollars and every amount is
rounded to the nearest cent.

## Main Feature: Planning the term

S-1 (UR-1): The system shall accept the money received only if it is a number
greater than $0 and at most $1,000,000.00.

S-2 (UR-2): The system shall accept an amount to set aside for saving from $0 up
to the money received. An empty box means $0.

S-3 (UR-1): The system shall require a term start date and a next transfer date
at least one day after the start date. The term covers the start date up to the
day before the next transfer; the transfer day belongs to the next term.

S-4 (UR-2): The system shall calculate the money the user can spend this term as
the money received minus the amount set aside for saving.

S-5 (UR-3): The system shall let the user change the money received, the amount
set aside, and the dates at any time. Recorded purchases are kept. New dates that
would leave one of this term's purchases outside the term are refused with a
message. Once the next transfer date has passed, a start date on or after it
starts a new term instead: the new term is saved as its own row in the budget
table, and last term's purchases stay with last term's row. A purchase belongs to
its term by budget_id, never by its date, so changing the new term's dates can
not bring last term's purchases into its totals, and the new term starts with
nothing to undo. Starting the next term before the transfer date is refused, so
the running term's purchases are never hidden early.

## Main Feature: Recording money in and out

S-6 (UR-4, UR-8): The system shall accept a purchase only when:

- the amount is a number greater than $0 and at most $1,000,000.00 (text like
  "$1,200.50" is accepted; commas are only allowed as thousands separators, so
  "12,50" is refused; zero, negative numbers, words, "1e5", NaN and infinity are
  not accepted),
- the date is on or after the term start date and before the next transfer date,
- the note is at most 100 characters (spaces at the start and end are removed; the
  note can be empty).

Anything else is rejected with a message that says what to fix, and nothing is
saved.

S-7 (UR-4): The system shall add a valid purchase to the list with an id that no
other saved purchase has, in this term or in any earlier term.

S-8 (UR-5): The system shall let the user correct the amount, date and note of a
recorded purchase. The same rules as S-6 apply; a rejected correction leaves the
purchase as it was.

S-9 (UR-6): The system shall remove a purchase when the user asks, without a
confirmation dialog, because the removal can be undone (S-10).

S-10 (UR-7): The system shall remember the user's most recent add, correction or
removal and reverse it when the user presses Undo. One step of undo is kept. An
undo that would bring back a purchase outside the current term dates is refused
with a message. After every add, correction, removal or undo, the system shows
the total spent and the money left right on the Expenses page.

S-11 (UR-8): The system shall show this term's purchases newest first (on the
same date, the one recorded last comes first), with date, note and amount. A new
purchase starts on today's date (or the term start date if the term has not
started yet).

## Main Feature: Checking where they stand

S-12 (UR-9): The system shall calculate the total spent as the sum of this term's
purchases each time it is shown (it is not stored), and the money left to spend
as the spendable money (S-4) minus the total spent. The money left can go below
zero.

S-13 (UR-10): The system shall work out the spending pace as the purchases dated
from the term start through today divided by the number of days from the term
start through today (today counts as a day). If the user keeps that pace, the
projected spending is everything recorded so far plus the pace times the days
left after today before the next transfer. The money lasts if the projected
spending is not more than the spendable money. If it does not last, the system
shows the date it runs out: today plus the money left divided by the pace,
rounded up to a whole day (today if nothing is left).

S-14 (UR-11): The system shall work out this week's amount. Weeks start on Monday,
except that the first week starts on the term start date. This week's amount is
the spendable money minus what was spent before this week, divided by the number
of weeks left including this one (a partial week counts as a week). What the user
can still spend this week is this week's amount minus what they spent this week,
and never more than the money left in the term.

S-15 (UR-3, UR-4 to UR-7): The system shall save the term and every purchase in a
SQLite database (budget.db in the user's app data folder) right after each change
and load them when the app starts, so nothing is lost when the app closes. The
budget table has one row per term, and every purchase row names the term it
belongs to, so a purchase belongs to its term by budget_id and not by its date.
The tables match the ER diagram in docs/.

S-16 (UR-9 to UR-11): The system shall show money with a dollar sign, two
decimals and thousands separators, with the minus sign before the dollar sign
(for example -$2,500.00). The money left is green when it is $0 or more and red
when it is below zero.

S-17 (UR-1): The system shall open on the Term setup tab when no term has been
set up yet, and on the Where I stand tab otherwise. When the next transfer date
has passed, the Where I stand tab says the term is over and points to Term setup
to start the next one.
