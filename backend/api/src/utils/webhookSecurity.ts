import crypto from 'crypto';

export function verifySquareSignature(rawBody: Buffer, signature: string | undefined, key: string): boolean {
  if (!signature || !key) return false;

  const expected = crypto.createHmac('sha256', key).update(rawBody).digest('base64');
  const actualBuffer = Buffer.from(signature);
  const expectedBuffer = Buffer.from(expected);
  if (actualBuffer.length !== expectedBuffer.length) return false;

  return crypto.timingSafeEqual(actualBuffer, expectedBuffer);
}

export function parseWebhookJson(rawBody: Buffer): unknown {
  return JSON.parse(rawBody.toString('utf8') || '{}');
}
