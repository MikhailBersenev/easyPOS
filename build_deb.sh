#!/bin/bash

# Скрипт для сборки deb пакета easyPOS
# Использование: ./build_deb.sh

set -e  # Остановка при ошибке

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Функция для вывода сообщений
info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Проверка, что скрипт запущен из правильной директории
if [ ! -f "CMakeLists.txt" ] || [ ! -d "debian" ]; then
    error "Скрипт должен быть запущен из корневой директории проекта easyPOS"
    exit 1
fi

info "Начинаем сборку deb пакета easyPOS..."

# Проверка зависимостей для сборки
info "Проверяем зависимости для сборки..."
MISSING_DEPS=""

check_dep() {
    if ! dpkg -l | grep -q "^ii.*$1"; then
        MISSING_DEPS="$MISSING_DEPS $1"
    fi
}

check_dep "build-essential"
check_dep "debhelper"
check_dep "cmake"
check_dep "qt6-base-dev"
check_dep "qt6-tools-dev"

if [ -n "$MISSING_DEPS" ]; then
    warn "Обнаружены отсутствующие зависимости:$MISSING_DEPS"
    read -p "Установить их автоматически? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        info "Устанавливаем зависимости..."
        sudo apt-get update
        sudo apt-get install -y build-essential debhelper cmake qt6-base-dev qt6-base-dev-tools qt6-tools-dev qt6-tools-dev-tools pkg-config
    else
        error "Установите зависимости вручную: sudo apt-get install build-essential debhelper cmake qt6-base-dev qt6-base-dev-tools qt6-tools-dev qt6-tools-dev-tools pkg-config"
        exit 1
    fi
else
    info "Все зависимости установлены"
fi

# Очистка предыдущих сборок (опционально)
if [ -d "../build" ] || ls ../easypos_*.deb 2>/dev/null | grep -q . || ls ../easypos_*.changes 2>/dev/null | grep -q .; then
    read -p "Очистить предыдущие сборки? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        info "Очищаем предыдущие сборки..."
        rm -rf ../build
        rm -f ../easypos_*.deb
        rm -f ../easypos_*.changes
        rm -f ../easypos_*.buildinfo
        rm -f ../easypos_*.dsc
        rm -f ../easypos_*.tar.*
        rm -rf debian/easypos
    fi
fi

# Очистка build директории проекта
if [ -d "build" ]; then
    info "Очищаем build директорию..."
    rm -rf build/*
fi

# Проверка версии из changelog
VERSION=$(head -1 debian/changelog | sed -n 's/.*(\([^)]*\)).*/\1/p')
info "Версия пакета: $VERSION"

# Сборка пакета
info "Запускаем сборку deb пакета..."
info "Это может занять некоторое время..."

if dpkg-buildpackage -b -us -uc; then
    info "Сборка завершена успешно!"
    
    # Поиск созданного пакета
    DEB_FILE=$(find .. -maxdepth 1 -name "easypos_*.deb" -type f | head -1)
    
    if [ -n "$DEB_FILE" ]; then
        info "Пакет создан: $DEB_FILE"
        
        # Показываем информацию о пакете
        info "Информация о пакете:"
        dpkg-deb -I "$DEB_FILE" | head -20
        
        # Показываем размер
        SIZE=$(du -h "$DEB_FILE" | cut -f1)
        info "Размер пакета: $SIZE"
        
        # Предложение установить
        echo
        read -p "Установить пакет? (y/n) " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            info "Устанавливаем пакет..."
            sudo dpkg -i "$DEB_FILE"
            
            # Проверка зависимостей
            if sudo apt-get install -f -y; then
                info "Пакет установлен успешно!"
            else
                warn "Возможны проблемы с зависимостями"
            fi
        fi
    else
        warn "Пакет не найден, но сборка завершилась успешно"
    fi
else
    error "Ошибка при сборке пакета"
    exit 1
fi

info "Готово!"

