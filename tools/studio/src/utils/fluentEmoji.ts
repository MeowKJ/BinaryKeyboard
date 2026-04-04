import {
  createFluentEmojiClient,
  getFluentEmojiMeta,
  type FluentEmojiUrlOptions,
} from '@meowkj/fluent-emoji-assets';

import cryingAnimated from '@/assets/emoji/crying_cat_animated.png';
import frogAnimated from '@/assets/emoji/frog_animated.png';
import grinningCatAnimated from '@/assets/emoji/grinning_cat_animated.png';
import grinningCatEyesAnimated from '@/assets/emoji/grinning_cat_with_smiling_eyes_animated.png';
import hourglassNotDoneAnimated from '@/assets/emoji/hourglass_not_done_animated.png';
import heartEyesAnimated from '@/assets/emoji/smiling_cat_with_heart-eyes_animated.png';
import wearyAnimated from '@/assets/emoji/weary_cat_animated.png';

export type StudioEmojiType =
  | 'cat'
  | 'grinning'
  | 'grinning-animated'
  | 'grinning-eyes-animated'
  | 'heart-eyes-animated'
  | 'crying-animated'
  | 'weary-animated'
  | 'hourglass-not-done-animated'
  | 'frog-3d'
  | 'frog-animated'
  | 'desktop-computer-3d'
  | 'crystal-ball-3d'
  | 'rabbit-face-3d'
  | 'cloud-lightning-3d';

type RemoteStudioEmoji = {
  kind: 'alias' | 'asset';
  name: string;
  fallback: string;
  options?: FluentEmojiUrlOptions;
};

type LocalStudioEmoji = {
  kind: 'local';
  fallback: string;
  src: string;
};

type StudioEmojiDefinition = RemoteStudioEmoji | LocalStudioEmoji;

export type ResolvedStudioEmoji = {
  fallback: string;
  src: string;
};

const fluentEmoji = createFluentEmojiClient({
  provider: 'jsdelivr',
  repo: {
    ref: 'main',
  },
});

const style3d = { style: '3D' } as const;

const aliasEmoji = (name: string, options?: FluentEmojiUrlOptions): RemoteStudioEmoji => ({
  kind: 'alias',
  name,
  fallback: getFluentEmojiMeta(name)?.fallback ?? '',
  options,
});

const assetEmoji = (
  name: string,
  fallback: string,
  options?: FluentEmojiUrlOptions,
): RemoteStudioEmoji => ({
  kind: 'asset',
  name,
  fallback,
  options,
});

const localEmoji = (src: string, fallback: string): LocalStudioEmoji => ({
  kind: 'local',
  src,
  fallback,
});

const studioEmojiCatalog: Record<StudioEmojiType, StudioEmojiDefinition> = {
  cat: assetEmoji('Cat face', '🐱', style3d),
  grinning: assetEmoji('Grinning cat', '😺', style3d),
  'grinning-animated': localEmoji(grinningCatAnimated, '😺'),
  'grinning-eyes-animated': localEmoji(grinningCatEyesAnimated, '😸'),
  'heart-eyes-animated': localEmoji(heartEyesAnimated, '😻'),
  'crying-animated': localEmoji(cryingAnimated, '😿'),
  'weary-animated': localEmoji(wearyAnimated, '🙀'),
  'hourglass-not-done-animated': localEmoji(hourglassNotDoneAnimated, '⏳'),
  'frog-3d': assetEmoji('Frog', '🐸', style3d),
  'frog-animated': localEmoji(frogAnimated, '🐸'),
  'desktop-computer-3d': aliasEmoji('desktopComputer', style3d),
  'crystal-ball-3d': assetEmoji('Crystal ball', '🔮', style3d),
  'rabbit-face-3d': assetEmoji('Rabbit face', '🐰', style3d),
  'cloud-lightning-3d': assetEmoji('Cloud with lightning', '🌩️', style3d),
};

export function getStudioEmoji(type: StudioEmojiType = 'cat'): ResolvedStudioEmoji {
  const definition = studioEmojiCatalog[type] ?? studioEmojiCatalog.cat;

  if (definition.kind === 'local') {
    return definition;
  }

  const src = definition.kind === 'alias'
    ? fluentEmoji.url(definition.name, definition.options)
    : fluentEmoji.rawUrl(definition.name, definition.options);

  return {
    src,
    fallback: definition.fallback,
  };
}
