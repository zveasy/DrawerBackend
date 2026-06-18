# International Deployment Notes

DrawerBackend inventory and twin models are generic across cash-heavy markets.
Inventory records support ISO-style three-letter currency codes, country and
region metadata, deployment environment, channel, and compliance tags.

## Cash Model

Inventory is represented by denomination name, currency code, quantity,
capacity, consumption rate, and last updated timestamp. The model supports
multiple currencies in a fleet by storing currency per denomination and at the
inventory summary level.

Currency codes are validated as three uppercase ASCII letters. Country, region,
denomination-set, and compliance tags are metadata fields so deployments can
model local cash handling rules without changing the core transaction engine.

## Transaction Metadata

International transaction records should carry:

- country or market region
- local denomination set
- currency code
- merchant and drawer identifiers
- local compliance tags

The implementation keeps these fields generic and does not encode country-
specific legal rules. Country-specific retention, receipt, KYC, agent banking,
cash-in/cash-out, or tax handling must be implemented as policy outside the
hardware runtime.

## Pilot Assumptions

The pilot should use one configured denomination set per drawer. Mixed-currency
drawers should be treated as a controlled operational mode until reconciliation,
cash handling, and compliance procedures are reviewed market by market.
