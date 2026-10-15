# Ввод

## Общее описание

Ввод проходит три ступени:

1. `Host_MainWndProc` (host.c) запоминает клавиатуру и мышь в полях `g_Window`;
2. главный цикл `WinMain` раз в такт передаёт их в `GmGame_Tick` и сбрасывает;
3. `GmInput_Dispatch` (objects.c) раздаёт ввод экземплярам в виде разрядов `GM_OBJECT.input` и запоминает тайлы под указателем и щелчка; скрипты читают их встроенными функциями.

За такт сохраняется только последнее событие, последняя клавиша направления и последний знак — более ранние в том же такте перезаписываются.

## Настройки

В `GM_SETTINGS` (`g_Settings`, сохраняется в автосохранении):

- `inputFlags` — разрешённые устройства: `GMDEV_KEYBOARD`, `GMDEV_MOUSE`; запрещённое устройство окно не обрабатывает;
- `dirKeys[8]` — виртуальные коды клавиш восьми направлений в порядке `GM_DIR8`: С, СВ, В, ЮВ, Ю, ЮЗ, З, СЗ; переназначаются `Native_SetDirKey`;
- `specialKeys[4]` — коды четырёх особых клавиш: действие 1, действие 2, следующий фокус, справка;
- `inputEnabled` — ввод включён (`Native_SetOption`/`Native_GetOption`). При выключенном вводе `GmGame_Tick` отбрасывает событие, клавишу и знак такта; положение мыши по-прежнему передаётся.

## Оконные сообщения

| Сообщение | Действие |
|-----------|---------|
| `WM_KEYDOWN` | клавиша направления → `g_Window.actionKey` = номер направления (`GMDIR8_N`…`GMDIR8_NW`), с Shift — плюс `GMDIR8_SHIFT_HELD`; точка щелчка — текущее положение мыши. Особая клавиша → `g_Window.event` = `GMEV_ACTION1`, `GMEV_ACTION2`, `GMEV_NEXT_FOCUS` или `GMEV_HELP`, `lastChar` = её код |
| `WM_CHAR` | `g_Window.lastChar` = знак |
| `WM_LBUTTONDOWN`, `WM_RBUTTONDOWN` | `event = GMEV_MOUSE_DOWN` (0x81) и точка щелчка — для обеих кнопок |
| `WM_LBUTTONUP` | `event = GMEV_ACTION1` (0x82), `lastChar = specialKeys[0]`, точка щелчка |
| `WM_RBUTTONUP` | `event = GMEV_ACTION2` (0x83), `lastChar = specialKeys[1]`, точка щелчка |
| `WM_MOUSEMOVE` | `g_Window.mouseX/Y`; `GmCursor_OnMouseMove` сразу перерисовывает курсор-спрайт |
| `WM_SETCURSOR` | в клиентской области — стрелка, если игра просит системный курсор (`systemCursor`), иначе курсор скрыт (`Host_GetCursor`) |

Таким образом, действие — это **отпускание** кнопки мыши или особая клавиша; нажатие кнопки — отдельное событие `GMEV_MOUSE_DOWN`. Отпускание кнопки, остановившей AVI, пропускается, чтобы оно не стало щелчком в игре.

Перед тактом `WinMain` проверяет, находится ли указатель в клиентской области (`cursorInWindow`; при выключенной мыши — всегда «да»).

## Раздача ввода — `GmInput_Dispatch`

`GmInput_Dispatch(mouseX, mouseY, clickX, clickY, event, actionKey)`:

1. Разряды ввода (`GM_INPUT_BITS`): `GMEV_ACTION1` → `GMIN_HOVER | GMIN_RELEASED`, `GMEV_ACTION2` → `GMIN_PRESSED | GMIN_RELEASED`; клавиша направления → `GMIN_KEY` | номер направления | `GMIN_SHIFT`. `GMEV_NEXT_FOCUS` переводит фокус (`GmFocus_Advance`). Без события точкой щелчка считается положение мыши.
2. Каждому экземпляру по виду класса:
   - `GMCLASS_ALL_INPUT` — все разряды;
   - `GMCLASS_POINTABLE` — только разряды клавиатуры;
   - `GMCLASS_FOCUSABLE` — разряды клавиатуры, если объект в фокусе, иначе 0.
3. Попадание: среди экземпляров, чей показываемый спрайт накрывает точку (`GmObject_HitTest`, границы включительно), выбирается объект с наименьшим слоем — он сверху.
4. Если попадания нет — тайл под указателем `g_PointerTileX/Y` считается по пикселям, `g_PointerTileClass` — класс этого тайла.
5. Если попадание есть — тайл под указателем = тайл объекта, а `g_PointerTileClass` получает **вид** класса объекта (особенность оригинала). Объект вида `GMCLASS_POINTABLE` получает все разряды плюс `GMIN_HOVER` (без события) или `GMIN_PRESSED` (при `GMEV_MOUSE_DOWN`). Объект вида `GMCLASS_FOCUSABLE` при щелчке действием получает фокус и разряды действия.
6. Для событий действия запоминается щелчок: `g_ClickEvent`, `g_ClickTileClass`, `g_ClickTileX/Y`; `GMEV_MOUSE_DOWN` стирает `g_ClickEvent`. Их читает поиск пути (см. [tilemap.md](tilemap.md)); `Native_PathAction(GMPATH_FORGET)` и `GmRoom_ResetView` их забывают.

`GMEV_HELP` движок сам не обрабатывает: справку вызывает скрипт (`Native_ShowHelp`), например по знаку особой клавиши.

## Фокус

Фокус клавиатуры — `g_FocusObject`, только у экземпляров вида `GMCLASS_FOCUSABLE`:

- первый созданный фокусируемый экземпляр получает фокус, если его ни у кого нет (`GmTile_Place`);
- `GmFocus_Advance(obj)` действует, только если `obj` — текущий фокус: фокус переходит к следующему фокусируемому экземпляру в порядке пула (или к первому); вызывается по `GMEV_NEXT_FOCUS`, при удалении объекта, при самозамене объекта в правиле и при входе в комнату;
- щелчок действием по фокусируемому объекту передаёт фокус ему.

## Чтение ввода из скриптов

| Функция | Условие |
|---------|---------|
| `Native_IsHovered` | `GMIN_HOVER` без `GMIN_RELEASED` — указатель над объектом |
| `Native_IsPressed` | `GMIN_PRESSED` без `GMIN_RELEASED` — кнопка нажата над объектом |
| `Native_MouseInputIs(bits)` | разряды мыши (`HOVER`, `PRESSED`, `RELEASED`) равны `bits`: действие 1 — `HOVER|RELEASED`, действие 2 — `PRESSED|RELEASED` |
| `Native_KeyInputIs(bits)` | разряды клавиатуры (`KEY`, `SHIFT`, направление) равны `bits` |
| `Native_IsDirectionKey(dirAndShift)` | нажата клавиша направления с учётом ориентации правила |
| `Native_InputCharIn(chars)` | знак такта (`g_InputChar`) входит в строку |

Кроме того, `GmSysVars_Update` публикует знак такта в системных переменных: `g_SysVars->keyPressed`, `g_SysVars->lastKey`.

Знак такта во время текстового ввода обрабатывает `GmTick_TextMode` (см. [ui.md](ui.md)).
