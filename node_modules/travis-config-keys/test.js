import { equal } from 'assert';
import travisConfigKeys from './index';

it('should travisConfigKeys', () =>
  equal(travisConfigKeys.length, 15));
