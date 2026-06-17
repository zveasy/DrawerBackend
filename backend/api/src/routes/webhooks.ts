import { Router, Request, Response } from 'express';
import { config } from '../utils/config';
import { logger } from '../utils/logger';
import { PrismaClient } from '@prisma/client';
import { handlePaymentEvent } from '../services/sweep';
import { parseWebhookJson, verifySquareSignature as verifySignature } from '../utils/webhookSecurity';
import { processSquareWebhookEvent } from '../utils/webhookProcessing';

const prisma = new PrismaClient();
export const webhooksRouter = Router();

function verifySquareSignature(req: Request): boolean {
  const raw = req.body instanceof Buffer ? req.body : Buffer.alloc(0);
  return verifySignature(raw, req.header('x-square-hmacsha256'), config.SQUARE_WEBHOOK_SIGNATURE_KEY);
}

webhooksRouter.post('/', async (req: Request, res: Response) => {
  try {
    if (!verifySquareSignature(req)) {
      logger.warn({ hasSignature: Boolean(req.header('x-square-hmacsha256')) }, 'invalid Square signature');
      return res.status(401).send('invalid signature');
    }

    const raw = req.body instanceof Buffer ? req.body : Buffer.alloc(0);
    const event: any = parseWebhookJson(raw);
    const result = await processSquareWebhookEvent(event, prisma, handlePaymentEvent);
    return res.status(result.status).send(result.body);
  } catch (err) {
    if (err instanceof SyntaxError) {
      logger.warn('malformed webhook payload');
      return res.status(400).send('malformed payload');
    }
    logger.error({ err }, 'webhook error');
    return res.status(500).send('error');
  }
});
