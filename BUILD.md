# Сборка Nautilus2

Инструкция для этого форка (Nautilus 52.alpha, GTK4 + libadwaita).

## Требования

- GTK **>= 4.24**
- GLib/GIO **>= 2.90**
- libadwaita **>= 1.8**
- meson **>= 1.1**, ninja, компилятор C
- blueprint-compiler **>= 0.19**

> Важно: из-за новых версий GTK/GLib/libadwaita собрать на старых стабильных
> дистрибутивах (например, Ubuntu 22.04/24.04) не получится без обновления
> системных библиотек. Проще всего собирать на Arch/Manjaro или в свежем Fedora.

### Arch / Manjaro

```bash
sudo pacman -S --needed base-devel git meson ninja blueprint-compiler \
  gtk4 libadwaita glib2 glycin glycin-gtk4 gnome-autoar gnome-desktop-4 \
  tinysparql libportal libportal-gtk4 gexiv2 gst-plugins-base-libs icu \
  libcloudproviders
```

### Fedora

```bash
sudo dnf install gcc gcc-c++ meson ninja-build git blueprint-compiler \
  gtk4-devel libadwaita-devel glib2-devel glycin-devel gnome-autoar-devel \
  gnome-desktop4-devel tinysparql-devel libportal-devel libportal-gtk4-devel \
  gexiv2-devel gstreamer1-plugins-base-devel libicu-devel libcloudproviders-devel
```

Если какого-то пакета нет — проверьте его имя через `dnf search <имя>`:
пакеты разработки в Fedora обычно называются `<библиотека>-devel`.

## Клонирование

```bash
git clone https://github.com/ArmanIOL/Nautilus2.git
cd Nautilus2
```

## Настройка и сборка

```bash
meson setup build --prefix=/usr --buildtype=release -Dselinux=false
ninja -C build
```

`-Dselinux=false` отключает SELinux-интеграцию (не нужна на большинстве
десктопов; если она нужна — уберите флаг и поставьте `libselinux`).

Сборка занимает несколько минут. Готовый бинарник появится в
`build/src/nautilus`.

## Запуск без установки

```bash
./build/src/nautilus
```

Так можно проверить изменения, не заменяя системный Nautilus. Учтите, что
часть функций (например, работа с порталами и D-Bus-сервисами) полноценно
работает только у установленной версии.

## Установка (замена системного Nautilus)

```bash
sudo ninja -C build install
```

После установки перезапустите файловый менеджер:

```bash
nautilus -q
```

и откройте «Файлы» заново.

## Обновление после `git pull`

```bash
git pull
ninja -C build
sudo ninja -C build install
```

Если менялись зависимости или опции meson:

```bash
meson setup --reconfigure build
```

## Полная пересборка

```bash
rm -rf build
meson setup build --prefix=/usr --buildtype=release -Dselinux=false
ninja -C build
sudo ninja -C build install
```

## Удаление

```bash
sudo ninja -C build uninstall
```

Затем переустановите системный пакет:

```bash
sudo pacman -S nautilus
```

(или `sudo dnf reinstall nautilus` / `sudo apt install --reinstall nautilus`).

## Если сборка падает

Посмотреть, чего не хватает:

```bash
meson setup build --wipe --prefix=/usr --buildtype=release -Dselinux=false
```

Сообщение вида `Dependency "gtk4" not found` означает, что не установлен
пакет разработки (`gtk4-devel`, `libgtk-4-dev` и т.п.) или его версия старее
требуемой.

Посмотреть подробный лог компиляции:

```bash
ninja -C build -v
```

## Что изменено в этом форке

Сайдбар перестроен в три секции:

1. **Точки входа** — Домашняя папка, Рабочий стол, Документы, Загрузки,
   Музыка, Изображения, Видео, Корзина (и закладки, если они есть).
2. **Сеть** — просмотр сети, облачные аккаунты и сетевые тома.
3. **Тома** — локальные диски, съёмные носители и точки монтирования.
