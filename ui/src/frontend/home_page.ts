// Copyright (C) 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

import * as m from 'mithril';

import { globals } from './globals';
import { createPage } from './pages';


export const HomePage = createPage({
  view() {
    return m(
      '.page.home-page',
      m(
        '.home-page-center',
        m('.home-page-title', 'magic-trace'),
        m(`img.logo[src=${globals.root}assets/logo-3d.png]`),
      ),
      /*
        m('a.privacy',
          {href: 'https://policies.google.com/privacy', target: '_blank'},
          'Privacy policy')
            m(
                'div.channel-select',
                m('div',
                  'Feeling adventurous? Try our bleeding edge Canary version'),
                m(
                    'fieldset',
                    mkChan('stable'),
                    mkChan('canary'),
                    m('.highlight'),
                    ),
                m(`.home-page-reload${channelChanged() ? '.show' : ''}`,
                  'You need to reload the page for the changes to have effect'),
                ),
      */
    );
  }
});
