type WebhookEventStore = {
  findUnique(args: { where: { id: string } }): Promise<unknown>;
  create(args: { data: { id: string; type: string } }): Promise<unknown>;
};

export type WebhookDb = {
  webhookEvent: WebhookEventStore;
};

export type WebhookHandler = (event: any, db: any) => Promise<void>;

export async function processSquareWebhookEvent(
  event: any,
  db: WebhookDb,
  paymentHandler?: WebhookHandler
): Promise<{ status: number; body: string }> {
  const eventId: string | undefined = event?.event_id || event?.id || event?.metadata?.event_id;
  if (!eventId) {
    return { status: 400, body: 'missing event_id' };
  }

  const existing = await db.webhookEvent.findUnique({ where: { id: eventId } });
  if (existing) {
    return { status: 200, body: 'ok' };
  }

  await db.webhookEvent.create({ data: { id: eventId, type: event?.type || 'unknown' } });

  if (paymentHandler && (event?.type?.startsWith('payment.') || event?.type?.startsWith('payments.'))) {
    await paymentHandler(event, db as any);
  }

  return { status: 200, body: 'ok' };
}
