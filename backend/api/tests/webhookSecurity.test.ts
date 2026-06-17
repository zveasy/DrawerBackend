import assert from 'assert';
import crypto from 'crypto';
import { verifySquareSignature, parseWebhookJson } from '../src/utils/webhookSecurity';
import { processSquareWebhookEvent } from '../src/utils/webhookProcessing';

function signatureFor(body: Buffer, key: string): string {
  return crypto.createHmac('sha256', key).update(body).digest('base64');
}

async function testSignatureValidation() {
  const key = 'test-secret';
  const body = Buffer.from(JSON.stringify({ event_id: 'evt-1' }));
  assert.equal(verifySquareSignature(body, signatureFor(body, key), key), true);
  assert.equal(verifySquareSignature(body, 'short', key), false);
  assert.equal(verifySquareSignature(body, undefined, key), false);
}

async function testMalformedPayload() {
  assert.throws(() => parseWebhookJson(Buffer.from('{bad json')), SyntaxError);
}

async function testDuplicateWebhookIdempotency() {
  const seen = new Set<string>(['evt-dup']);
  let createCalls = 0;
  const db: any = {
    webhookEvent: {
      findUnique: async ({ where }: any) => (seen.has(where.id) ? { id: where.id } : null),
      create: async ({ data }: any) => {
        createCalls += 1;
        seen.add(data.id);
        return data;
      },
    },
  };

  const duplicate = await processSquareWebhookEvent({ event_id: 'evt-dup', type: 'payment.created' }, db);
  assert.equal(duplicate.status, 200);
  assert.equal(createCalls, 0);

  const fresh = await processSquareWebhookEvent({ event_id: 'evt-new', type: 'payment.created' }, db, async () => {});
  assert.equal(fresh.status, 200);
  assert.equal(createCalls, 1);
}

async function testMissingProductionSecretConfig() {
  const previousNodeEnv = process.env.NODE_ENV;
  const previousSecret = process.env.SQUARE_WEBHOOK_SIGNATURE_KEY;
  process.env.NODE_ENV = 'production';
  delete process.env.SQUARE_WEBHOOK_SIGNATURE_KEY;
  const path = require.resolve('../src/utils/config');
  delete require.cache[path];
  assert.throws(() => require('../src/utils/config'), /SQUARE_WEBHOOK_SIGNATURE_KEY/);
  delete require.cache[path];
  if (previousNodeEnv === undefined) delete process.env.NODE_ENV;
  else process.env.NODE_ENV = previousNodeEnv;
  if (previousSecret === undefined) delete process.env.SQUARE_WEBHOOK_SIGNATURE_KEY;
  else process.env.SQUARE_WEBHOOK_SIGNATURE_KEY = previousSecret;
}

async function main() {
  await testSignatureValidation();
  await testMalformedPayload();
  await testDuplicateWebhookIdempotency();
  await testMissingProductionSecretConfig();
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
