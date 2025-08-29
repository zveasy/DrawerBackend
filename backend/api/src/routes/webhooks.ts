import { Router, Request, Response } from 'express';
import crypto from 'crypto';
import { config } from '../utils/config';
import { logger } from '../utils/logger';
import { PrismaClient } from '@prisma/client';
import { handlePaymentEvent } from '../services/sweep';

const prisma = new PrismaClient();
export const webhooksRouter = Router();

function verifySquareSignature(req: Request): boolean {
  // Square Webhooks v2: header 'x-square-hmacsha256' contains base64(HMAC_SHA256(signature_key, raw_body))
  const signature = req.header('x-square-hmacsha256');
  if (!signature) return false;

  const body = req.body instanceof Buffer ? req.body.toString('utf8') : '';

  const hmac = crypto.createHmac('sha256', config.SQUARE_WEBHOOK_SIGNATURE_KEY);
  hmac.update(body);
  const expected = hmac.digest('base64');

  return crypto.timingSafeEqual(Buffer.from(signature), Buffer.from(expected));
}

let warnedNoSigKey = false;

webhooksRouter.post('/', async (req: Request, res: Response) => {
  try {
    if (!verifySquareSignature(req)) {
      logger.warn({ headers: req.headers }, 'invalid Square signature');
      return res.status(401).send('invalid signature');
    }

    const raw = req.body instanceof Buffer ? req.body.toString('utf8') : '';
    const event = JSON.parse(raw || '{}');

    const eventId: string | undefined = event?.event_id || event?.id || event?.metadata?.event_id;
    if (!eventId) {
      return res.status(400).send('missing event_id');
    }

    // Idempotency guard
    const existing = await prisma.webhookEvent.findUnique({ where: { id: eventId } });
    if (existing) {
      return res.status(200).send('ok');
    }

    await prisma.webhookEvent.create({ data: { id: eventId, type: event?.type || 'unknown' } });

    // Handle payments events
    if (event?.type?.startsWith('payment.') || event?.type?.startsWith('payments.')) {
      await handlePaymentEvent(event, prisma);
    }

    return res.status(200).send('ok');
  } catch (err) {
    logger.error({ err }, 'webhook error');
    return res.status(500).send('error');
  }
});
