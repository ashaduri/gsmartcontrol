---
title: "What Is Smart?"
permalink: /what-is-smart
---

# What is SMART?

SMART is a technology which provides hard disk drives with methods to
predict certain kinds of failures with certain chance of success.

## A Detailed Answer

Self-Monitoring, Analysis, and Reporting Technology, or SMART, is a
monitoring system for hard drives to detect and report various indicators
of reliability, in the hope of anticipating failures. SMART is implemented
inside the drives, providing several ways of monitoring the drive health.
It may present information about general health, various drive attributes
(for example, number of unreadable sectors), error logs, and so on.
It may also provide ways to instruct the drive to run various self-tests,
which may report valuable information. It may even automatically scan
the disk surface in when the drive is idle, repairing the defects while
reallocating the data to more safe areas.

While having SMART sounds perfect, there are some nuances to
consider. One of the common pitfalls is that it may create a false sense
of security. That is, a perfectly good SMART data is NOT an indication
that the drive won't fail the next minute. The reverse is also true - some
drives may function perfectly even with not-so-good-looking SMART
data. However, as studies indicate, given a large population of drives,
some SMART attributes may reliably predict drive failures within up to
two months.

Another common mistake is to assume that the attribute values are
the real physical values, as experienced by the drive. As manufacturers
do not necessarily agree on precise attribute definitions and
measurement units, the exact meaning of the attributes may vary
greatly across different drive models.

At present SMART is implemented individually by manufacturers.
While some aspects are standardized for compatibility, others are not.
In fact, most manufacturers refer the users to their own health
monitoring utilities and advice against taking SMART data seriously.
Nevertheless, SMART may prove an effective measure against data loss.

Yet another issue is that quite often the drives have bugs which
prevent correct SMART usage. This is usually due to buggy firmware,
or the manufacturer ignoring the standards. Luckily, smartmontools
usually detects these bugs and works around them.
