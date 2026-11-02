# MVP - User Requirements

Who this is for: a college student who lives on money their family sends once a
term (or once a month) and wants to know how much they can spend until the next
transfer arrives.

Each requirement says what the user should be able to do. How the system does it
is in [SPECIFICATIONS.md](SPECIFICATIONS.md).

## Main Feature: Planning the term

UR-1: The user should be able to enter how much money they received for this
term, the day the term started, and the date the next transfer will arrive.

UR-2: The user should be able to set aside part of that money for saving, so it
is not counted as money they can spend.

UR-3: The user should be able to change the money received, the amount set
aside, or the dates later if they typed something wrong or their plans change,
and start the next term when the next transfer arrives.

## Main Feature: Recording money in and out

UR-4: The user should be able to record a purchase with its amount, the date
they made it, and a short note about what it was for.

UR-5: The user should be able to correct a purchase they recorded wrong (amount,
date, or note).

UR-6: The user should be able to remove a purchase they recorded by mistake.

UR-7: The user should be able to undo their last add, correction, or removal if
they did it by accident.

UR-8: The user should be able to see the purchases they recorded, and be told
clearly when a purchase they typed cannot be saved and why (for example an amount
of $0 or a date outside the term).

## Main Feature: Checking where they stand

UR-9: The user should be able to see how much they have spent so far and how much
of the term's money is left to spend, updated as soon as they record, correct,
or remove a purchase.

UR-10: The user should be able to see whether their money will last until the
next transfer if they keep spending at their current pace, and if not, about
when it will run out.

UR-11: The user should be able to see how much they can still spend this week.
