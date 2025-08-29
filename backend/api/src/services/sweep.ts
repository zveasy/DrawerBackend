import { PrismaClient, RuleType } from '@prisma/client';
import { logger } from '../utils/logger.js';

export async function handlePaymentEvent(event: any, prisma: PrismaClient) {
  // Square payments event shape
  const merchantId: string | undefined = event?.merchant_id;
  const payment = event?.data?.object?.payment || event?.data?.payment;
  if (!merchantId || !payment) {
    logger.warn({ eventType: event?.type }, 'payment event missing fields');
    return;
  }

  const squarePaymentId: string = payment.id;
  const amountCents: number = Number(payment?.amount_money?.amount ?? 0);
  const currency: string = payment?.amount_money?.currency || 'USD';

  // Ensure Merchant and Drawer exist
  const merchant = await prisma.merchant.upsert({
    where: { squareMerchantId: merchantId },
    update: {},
    create: { squareMerchantId: merchantId },
  });

  const drawer = await prisma.drawer.upsert({
    where: { merchantId: merchant.id },
    update: {},
    create: { merchantId: merchant.id, currency },
  });

  // Record transaction if new
  const trx = await prisma.transaction.upsert({
    where: { squarePaymentId: squarePaymentId },
    update: {},
    create: {
      merchantId: merchant.id,
      squarePaymentId,
      amountCents,
      currency,
    },
  });

  // Find active sweep rule (default to 1% if none exists)
  let rule = await prisma.sweepRule.findFirst({ where: { drawerId: drawer.id, active: true } });
  if (!rule) {
    rule = await prisma.sweepRule.create({
      data: { drawerId: drawer.id, type: 'PERCENT' as RuleType, value: 100 }, // 100 bps = 1%
    });
  }

  let sweepCents = 0;
  if (rule.type === 'PERCENT') {
    sweepCents = Math.floor((amountCents * rule.value) / 10000); // value in bps
  } else {
    sweepCents = Math.min(amountCents, rule.value);
  }

  if (sweepCents <= 0) {
    return;
  }

  // Ledger entry + drawer balance update atomically
  await prisma.$transaction([
    prisma.ledgerEntry.create({
      data: { drawerId: drawer.id, type: 'SWEEP', amountCents: sweepCents, refId: squarePaymentId },
    }),
    prisma.drawer.update({ where: { id: drawer.id }, data: { balanceCents: { increment: sweepCents } } }),
  ]);

  logger.info({ paymentId: squarePaymentId, sweepCents }, 'sweep recorded');
}
