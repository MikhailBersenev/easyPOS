#ifndef ALERTKEYS_H
#define ALERTKEYS_H

/**
 * Обёрточные константы для событий ИС (по аналогии с SettingsKeys).
 * Использование: alerts->log(AlertCategory::Auth, AlertSignature::LoginSuccess, fullLog, userId, employeeId);
 */

namespace AlertCategory {
    // Аутентификация и сессии
    constexpr const char *Auth = "auth";
    // Продажи и чеки
    constexpr const char *Sales = "sales";
    // Рабочие смены
    constexpr const char *Shift = "shift";
    // Производство (рецепты, запуски)
    constexpr const char *Production = "production";
    // Склад и остатки
    constexpr const char *Stock = "stock";
    // Справочники (товары, категории, услуги и т.д.)
    constexpr const char *Reference = "reference";
    // Системные события (подключение БД, настройки, ошибки)
    constexpr const char *System = "system";
}

namespace AlertSignature {
    // --- Auth ---
    constexpr const char *LoginSuccess = "login_success";
    constexpr const char *LoginFailed = "login_failed";
    constexpr const char *Logout = "logout";
    constexpr const char *SessionExpired = "session_expired";

    // --- Sales ---
    constexpr const char *CheckCreated = "check_created";
    constexpr const char *CheckFinalized = "check_finalized";
    constexpr const char *CheckCancelled = "check_cancelled";
    constexpr const char *SaleAdded = "sale_added";
    constexpr const char *SaleRemoved = "sale_removed";
    constexpr const char *DiscountApplied = "discount_applied";
    constexpr const char *PaymentRecorded = "payment_recorded";

    // --- Shift ---
    constexpr const char *ShiftStarted = "shift_started";
    constexpr const char *ShiftEnded = "shift_ended";

    // --- Production ---
    constexpr const char *ProductionRunStarted = "production_run_started";
    constexpr const char *ProductionRunCompleted = "production_run_completed";

    // --- Stock ---
    constexpr const char *StockReserved = "stock_reserved";
    constexpr const char *StockReleased = "stock_released";
    constexpr const char *StockRemoved = "stock_removed";

    // --- Reference ---
    constexpr const char *GoodCreated = "good_created";
    constexpr const char *GoodUpdated = "good_updated";
    constexpr const char *GoodDeleted = "good_deleted";
    constexpr const char *CategoryUpdated = "category_updated";
    constexpr const char *UserCreated = "user_created";
    constexpr const char *UserUpdated = "user_updated";
    constexpr const char *UserDeleted = "user_deleted";
    constexpr const char *UserRestored = "user_restored";
    constexpr const char *RoleCreated = "role_created";
    constexpr const char *RoleUpdated = "role_updated";
    constexpr const char *RoleDeleted = "role_deleted";
    constexpr const char *RoleBlocked = "role_blocked";
    constexpr const char *EmployeeCreated = "employee_created";
    constexpr const char *EmployeeUpdated = "employee_updated";
    constexpr const char *EmployeeDeleted = "employee_deleted";
    constexpr const char *PositionCreated = "position_created";
    constexpr const char *PositionUpdated = "position_updated";
    constexpr const char *PositionDeleted = "position_deleted";
    constexpr const char *PromotionCreated = "promotion_created";
    constexpr const char *PromotionUpdated = "promotion_updated";
    constexpr const char *PromotionDeleted = "promotion_deleted";

    // --- System ---
    constexpr const char *DbConnected = "db_connected";
    constexpr const char *DbDisconnected = "db_disconnected";
    constexpr const char *SettingsChanged = "settings_changed";
    constexpr const char *DbSettingsChanged = "db_settings_changed";
    constexpr const char *Error = "error";
}

#endif // ALERTKEYS_H
