// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/prefs/browser_prefs.h"

#include <array>
#include <optional>
#include <string>
#include <string_view>

#include "ash/constants/ash_constants.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "build/branding_buildflags.h"
#include "build/build_config.h"
#include "build/chromecast_buildflags.h"
#include "build/chromeos_buildflags.h"
#include "chrome/browser/about_flags.h"
#include "chrome/browser/accessibility/accessibility_labels_service.h"
#include "chrome/browser/accessibility/invert_bubble_prefs.h"
#include "chrome/browser/accessibility/prefers_default_scrollbar_styles_prefs.h"
#include "chrome/browser/browser_process_impl.h"
#include "chrome/browser/chrome_content_browser_client.h"
#include "chrome/browser/chromeos/enterprise/cloud_storage/policy_utils.h"
#include "chrome/browser/chromeos/upload_office_to_cloud/upload_office_to_cloud.h"
#include "chrome/browser/component_updater/component_updater_prefs.h"
#include "chrome/browser/devtools/devtools_window.h"
#include "chrome/browser/download/download_prefs.h"
#include "chrome/browser/engagement/important_sites_util.h"
#include "chrome/browser/enterprise/connectors/connectors_prefs.h"
#include "chrome/browser/enterprise/reporting/prefs.h"
#include "chrome/browser/enterprise/util/managed_browser_utils.h"
#include "chrome/browser/external_protocol/external_protocol_handler.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/gpu/gpu_mode_manager.h"
#include "chrome/browser/lifetime/browser_shutdown.h"
#include "chrome/browser/login_detection/login_detection_prefs.h"
#include "chrome/browser/media/media_engagement_service.h"
#include "chrome/browser/media/media_storage_id_salt.h"
#include "chrome/browser/media/prefs/capture_device_ranking.h"
#include "chrome/browser/media/router/discovery/access_code/access_code_cast_feature.h"
#include "chrome/browser/media/router/media_router_feature.h"
#include "chrome/browser/media/webrtc/capture_policy_utils.h"
#include "chrome/browser/media/webrtc/media_capture_devices_dispatcher.h"
#include "chrome/browser/media/webrtc/permission_bubble_media_access_handler.h"
#include "chrome/browser/memory/enterprise_memory_limit_pref_observer.h"
#include "chrome/browser/metrics/chrome_metrics_service_client.h"
#include "chrome/browser/net/net_error_tab_helper.h"
#include "chrome/browser/net/profile_network_context_service.h"
#include "chrome/browser/net/secure_dns_util.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/notifications/notification_display_service_impl.h"
#include "chrome/browser/notifications/notifier_state_tracker.h"
#include "chrome/browser/notifications/platform_notification_service_impl.h"
#include "chrome/browser/permissions/quiet_notification_permission_ui_state.h"
#include "chrome/browser/prefs/chrome_pref_service_factory.h"
#include "chrome/browser/prefs/incognito_mode_prefs.h"
#include "chrome/browser/prefs/session_startup_pref.h"
#include "chrome/browser/preloading/prefetch/prefetch_service/prefetch_origin_decider.h"
#include "chrome/browser/preloading/prefetch/search_prefetch/search_prefetch_service.h"
#include "chrome/browser/preloading/preloading_prefs.h"
#include "chrome/browser/printing/print_preview_sticky_settings.h"
#include "chrome/browser/profiles/chrome_version_service.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_impl.h"
#include "chrome/browser/profiles/profiles_state.h"
#include "chrome/browser/push_messaging/push_messaging_app_identifier.h"
#include "chrome/browser/push_messaging/push_messaging_service_impl.h"
#include "chrome/browser/rlz/chrome_rlz_tracker_delegate.h"
#include "chrome/browser/search/search.h"
#include "chrome/browser/sharing/sharing_sync_preference.h"
#include "chrome/browser/sharing_hub/sharing_hub_features.h"
#include "chrome/browser/ssl/ssl_config_service_manager.h"
#include "chrome/browser/task_manager/task_manager_interface.h"
#include "chrome/browser/tpcd/experiment/tpcd_pref_names.h"
#include "chrome/browser/tracing/chrome_tracing_delegate.h"
#include "chrome/browser/ui/browser_ui_prefs.h"
#include "chrome/browser/ui/hats/hats_service_desktop.h"
#include "chrome/browser/ui/network_profile_bubble.h"
#include "chrome/browser/ui/prefs/prefs_tab_helper.h"
#include "chrome/browser/ui/search_engines/keyword_editor_controller.h"
#include "chrome/browser/ui/send_tab_to_self/send_tab_to_self_bubble.h"
#include "chrome/browser/ui/side_panel/side_panel_prefs.h"
#include "chrome/browser/ui/tabs/organization/prefs.h"
#include "chrome/browser/ui/tabs/pinned_tab_codec.h"
#include "chrome/browser/ui/toolbar/chrome_labs/chrome_labs_prefs.h"
#include "chrome/browser/ui/toolbar/chrome_location_bar_model_delegate.h"
#include "chrome/browser/ui/toolbar/toolbar_pref_names.h"
#include "chrome/browser/ui/webui/accessibility/accessibility_ui.h"
#include "chrome/browser/ui/webui/bookmarks/bookmark_prefs.h"
#include "chrome/browser/ui/webui/flags/flags_ui.h"
#include "chrome/browser/ui/webui/ntp/new_tab_ui.h"
#include "chrome/browser/ui/webui/policy/policy_ui.h"
#include "chrome/browser/ui/webui/print_preview/policy_settings.h"
#include "chrome/browser/updates/announcement_notification/announcement_notification_service.h"
#include "chrome/browser/user_education/browser_feature_promo_storage_service.h"
#include "chrome/browser/webauthn/chrome_authenticator_request_delegate.h"
#include "chrome/browser/webauthn/webauthn_pref_names.h"
#include "chrome/common/buildflags.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/secure_origin_allowlist.h"
#include "components/autofill/core/common/autofill_prefs.h"
#include "components/blocked_content/safe_browsing_triggered_popup_blocker.h"
#include "components/breadcrumbs/core/breadcrumbs_status.h"
#include "components/browser_sync/sync_to_signin_migration.h"
#include "components/browsing_data/core/pref_names.h"
#include "components/certificate_transparency/pref_names.h"
#include "components/commerce/core/pref_names.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/pref_names.h"
#include "components/custom_handlers/protocol_handler_registry.h"
#include "components/dom_distiller/core/distilled_page_prefs.h"
#include "components/domain_reliability/domain_reliability_prefs.h"
#include "components/embedder_support/origin_trials/origin_trial_prefs.h"
#include "components/enterprise/browser/identifiers/identifiers_prefs.h"
#include "components/enterprise/buildflags/buildflags.h"
#include "components/flags_ui/pref_service_flags_storage.h"
#include "components/history_clusters/core/history_clusters_prefs.h"
#include "components/image_fetcher/core/cache/image_cache.h"
#include "components/invalidation/impl/fcm_invalidation_service.h"
#include "components/invalidation/impl/invalidator_registrar_with_memory.h"
#include "components/invalidation/impl/per_user_topic_subscription_manager.h"
#include "components/language/content/browser/geo_language_provider.h"
#include "components/language/content/browser/ulp_language_code_locator/ulp_language_code_locator.h"
#include "components/language/core/browser/language_prefs.h"
#include "components/lens/buildflags.h"
#include "components/lookalikes/core/lookalike_url_util.h"
#include "components/media_device_salt/media_device_id_salt.h"
#include "components/metrics/demographics/user_demographics.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/network_time/network_time_tracker.h"
#include "components/ntp_tiles/most_visited_sites.h"
#include "components/offline_pages/buildflags/buildflags.h"
#include "components/omnibox/browser/document_provider.h"
#include "components/omnibox/browser/omnibox_prefs.h"
#include "components/omnibox/browser/zero_suggest_provider.h"
#include "components/optimization_guide/core/model_execution/model_execution_prefs.h"
#include "components/optimization_guide/core/optimization_guide_prefs.h"
#include "components/password_manager/core/browser/password_manager.h"
#include "components/payments/core/payment_prefs.h"
#include "components/performance_manager/public/user_tuning/prefs.h"
#include "components/permissions/permission_hats_trigger_helper.h"
#include "components/permissions/pref_names.h"
#include "components/plus_addresses/plus_address_prefs.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/browser/url_blocklist_manager.h"
#include "components/policy/core/common/local_test_policy_provider.h"
#include "components/policy/core/common/management/management_service.h"
#include "components/policy/core/common/policy_pref_names.h"
#include "components/policy/core/common/policy_statistics_collector.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/privacy_sandbox/privacy_sandbox_prefs.h"
#include "components/proxy_config/pref_proxy_config_tracker_impl.h"
#include "components/safe_browsing/content/common/file_type_policies_prefs.h"
#include "components/safe_browsing/core/common/safe_browsing_prefs.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "components/security_interstitials/content/insecure_form_blocking_page.h"
#include "components/security_interstitials/content/stateful_ssl_host_state_delegate.h"
#include "components/segmentation_platform/embedder/default_model/device_switcher_result_dispatcher.h"
#include "components/segmentation_platform/public/segmentation_platform_service.h"
#include "components/services/storage/public/cpp/storage_prefs.h"
#include "components/sessions/core/session_id_generator.h"
#include "components/signin/public/base/signin_pref_names.h"
#include "components/signin/public/identity_manager/identity_manager.h"
#include "components/site_engagement/content/site_engagement_service.h"
#include "components/subresource_filter/content/browser/ruleset_service.h"
#include "components/subresource_filter/core/browser/subresource_filter_constants.h"
#include "components/supervised_user/core/browser/supervised_user_preferences.h"
#include "components/supervised_user/core/common/buildflags.h"
#include "components/sync/base/pref_names.h"
#include "components/sync/service/glue/sync_transport_data_prefs.h"
#include "components/sync/service/sync_prefs.h"
#include "components/sync_device_info/device_info_prefs.h"
#include "components/sync_preferences/pref_service_syncable.h"
#include "components/sync_sessions/session_sync_prefs.h"
#include "components/tracing/common/pref_names.h"
#include "components/translate/core/browser/translate_prefs.h"
#include "components/update_client/update_client.h"
#include "components/variations/service/variations_service.h"
#include "content/public/browser/render_process_host.h"
#include "extensions/buildflags/buildflags.h"
#include "net/http/http_server_properties_manager.h"
#include "pdf/buildflags.h"
#include "ppapi/buildflags/buildflags.h"
#include "printing/buildflags/buildflags.h"
#include "rlz/buildflags/buildflags.h"
#include "services/screen_ai/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_BACKGROUND_MODE)
#include "chrome/browser/background/background_mode_manager.h"
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/accessibility/animation_policy_prefs.h"
#include "chrome/browser/apps/platform_apps/shortcut_manager.h"
#include "chrome/browser/extensions/activity_log/activity_log.h"
#include "chrome/browser/extensions/api/commands/command_service.h"
#include "chrome/browser/extensions/api/tabs/tabs_api.h"
#include "chrome/browser/extensions/extension_web_ui.h"
#include "chrome/browser/extensions/preinstalled_apps.h"
#include "chrome/browser/ui/extensions/settings_api_bubble_helpers.h"
#include "chrome/browser/ui/webui/extensions/extensions_ui.h"
#include "extensions/browser/api/audio/audio_api.h"
#include "extensions/browser/api/runtime/runtime_api.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/permissions_manager.h"
#include "extensions/browser/pref_names.h"
#if BUILDFLAG(IS_CHROMEOS_ASH)
#include "chrome/browser/ash/crosapi/browser_data_migrator.h"
#include "chrome/browser/ash/device_name/device_name_store.h"
#include "chrome/browser/ash/extensions/extensions_permissions_tracker.h"
#include "chrome/browser/ash/kerberos/kerberos_credentials_manager.h"
#include "chrome/browser/ash/net/system_proxy_manager.h"
#include "chrome/browser/ash/platform_keys/key_permissions/key_permissions_manager_impl.h"
#include "chrome/browser/ash/policy/networking/euicc_status_uploader.h"
#include "chrome/browser/ash/policy/remote_commands/crd/crd_admin_session_controller.h"
#include "chrome/browser/ash/settings/hardware_data_usage_controller.h"
#include "chrome/browser/ash/settings/stats_reporting_controller.h"
#include "chrome/browser/component_updater/metadata_table_chromeos.h"
#include "chrome/browser/extensions/api/shared_storage/shared_storage_private_api.h"
#include "chrome/browser/ui/ash/projector/projector_app_client_impl.h"
#include "chrome/browser/ui/webui/ash/edu_coexistence/edu_coexistence_login_handler.h"
#include "chrome/browser/ui/webui/signin/ash/inline_login_handler_impl.h"
#include "chromeos/ash/components/carrier_lock/carrier_lock_manager.h"
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

#if BUILDFLAG(ENABLE_PDF)
#include "chrome/browser/pdf/pdf_pref_names.h"
#endif  // BUILDFLAG(ENABLE_PDF)

#if BUILDFLAG(ENABLE_SCREEN_AI_SERVICE)
#include "chrome/browser/screen_ai/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_SUPERVISED_USERS)
#include "components/supervised_user/core/browser/child_account_service.h"
#include "components/supervised_user/core/browser/supervised_user_service.h"
#endif

#include "components/feed/buildflags.h"
#include "components/feed/core/common/pref_names.h"
#include "components/feed/core/shared_prefs/pref_names.h"

#if BUILDFLAG(IS_ANDROID)
#include "chrome/browser/accessibility/accessibility_prefs/android/accessibility_prefs_controller.h"
#include "chrome/browser/android/bookmarks/partner_bookmarks_shim.h"
#include "chrome/browser/android/ntp/recent_tabs_page_prefs.h"
#include "chrome/browser/android/oom_intervention/oom_intervention_decider.h"
#include "chrome/browser/android/preferences/browser_prefs_android.h"
#include "chrome/browser/android/preferences/shared_preferences_migrator_android.h"
#include "chrome/browser/android/usage_stats/usage_stats_bridge.h"
#include "chrome/browser/first_run/android/first_run_prefs.h"
#include "chrome/browser/lens/android/lens_prefs.h"
#include "chrome/browser/media/android/cdm/media_drm_origin_id_manager.h"
#include "chrome/browser/notifications/notification_channels_provider_android.h"
#include "chrome/browser/password_manager/android/password_manager_android_util.h"
#include "chrome/browser/readaloud/android/prefs.h"
#include "chrome/browser/ssl/known_interception_disclosure_infobar_delegate.h"
#include "components/cdm/browser/media_drm_storage_impl.h"  // nogncheck crbug.com/1125897
#include "components/ntp_snippets/register_prefs.h"
#include "components/ntp_tiles/popular_sites_impl.h"
#include "components/permissions/contexts/geolocation_permission_context_android.h"
#include "components/query_tiles/tile_service_prefs.h"
#include "components/webapps/browser/android/install_prompt_prefs.h"
#else  // BUILDFLAG(IS_ANDROID)
#include "chrome/browser/cart/cart_service.h"
#include "chrome/browser/companion/core/promo_handler.h"
#include "chrome/browser/device_api/device_service_impl.h"
#include "chrome/browser/gcm/gcm_product_util.h"
#include "chrome/browser/hid/hid_policy_allowed_devices.h"
#include "chrome/browser/intranet_redirect_detector.h"
#include "chrome/browser/media/unified_autoplay_config.h"
#include "chrome/browser/metrics/tab_stats/tab_stats_tracker.h"
#include "chrome/browser/nearby_sharing/common/nearby_share_prefs.h"
#include "chrome/browser/new_tab_page/modules/drive/drive_service.h"
#include "chrome/browser/new_tab_page/modules/photos/photos_service.h"
#include "chrome/browser/new_tab_page/modules/recipes/recipes_service.h"
#include "chrome/browser/new_tab_page/modules/safe_browsing/safe_browsing_handler.h"
#include "chrome/browser/new_tab_page/modules/v2/tab_resumption/tab_resumption_page_handler.h"
#include "chrome/browser/new_tab_page/promos/promo_service.h"
#include "chrome/browser/policy/developer_tools_policy_handler.h"
#include "chrome/browser/search/background/ntp_custom_background_service.h"
#include "chrome/browser/search_engine_choice/search_engine_choice_client_side_trial.h"
#include "chrome/browser/search_engine_choice/search_engine_choice_dialog_service.h"
#include "chrome/browser/serial/serial_policy_allowed_ports.h"
#include "chrome/browser/signin/signin_promo.h"
#include "chrome/browser/ui/commerce/commerce_ui_tab_helper.h"
#include "chrome/browser/ui/safety_hub/safety_hub_prefs.h"
#include "chrome/browser/ui/startup/startup_browser_creator.h"
#include "chrome/browser/ui/webui/cr_components/theme_color_picker/theme_color_picker_handler.h"
#include "chrome/browser/ui/webui/history/foreign_session_handler.h"
#include "chrome/browser/ui/webui/new_tab_page/new_tab_page_handler.h"
#include "chrome/browser/ui/webui/new_tab_page/new_tab_page_ui.h"
#include "chrome/browser/ui/webui/settings/settings_ui.h"
#include "chrome/browser/ui/webui/side_panel/read_anything/read_anything_prefs.h"
#include "chrome/browser/ui/webui/tab_search/tab_search_prefs.h"
#include "chrome/browser/ui/webui/whats_new/whats_new_ui.h"
#include "chrome/browser/upgrade_detector/upgrade_detector.h"
#include "components/headless/policy/headless_mode_prefs.h"
#include "components/live_caption/live_caption_controller.h"
#include "components/live_caption/live_translate_controller.h"
#include "components/ntp_tiles/custom_links_manager_impl.h"
#include "components/user_notes/user_notes_prefs.h"
#endif  // BUILDFLAG(IS_ANDROID)

#if !BUILDFLAG(IS_ANDROID) && BUILDFLAG(GOOGLE_CHROME_BRANDING)
#include "chrome/browser/promos/promos_utils.h"
#endif  // !BUILDFLAG(IS_ANDROID) && BUILDFLAG(GOOGLE_CHROME_BRANDING)

#if BUILDFLAG(IS_CHROMEOS)
#include "chrome/browser/chromeos/extensions/echo_private/echo_private_api.h"
#include "chrome/browser/chromeos/extensions/login_screen/login/login_api_prefs.h"
#include "chrome/browser/chromeos/policy/dlp/dlp_rules_manager_impl.h"
#include "chrome/browser/chromeos/quickoffice/quickoffice_prefs.h"
#include "chrome/browser/chromeos/reporting/metric_reporting_prefs.h"
#include "chrome/browser/extensions/api/document_scan/document_scan_api_handler.h"
#include "chrome/browser/extensions/api/enterprise_platform_keys/enterprise_platform_keys_api.h"
#include "chrome/browser/memory/oom_kills_monitor.h"
#include "chrome/browser/policy/networking/policy_cert_service.h"
#include "chrome/browser/policy/system_features_disable_list_policy_handler.h"
#include "chrome/browser/ui/webui/certificates_handler.h"
#include "chromeos/ui/wm/fullscreen/pref_names.h"
#if BUILDFLAG(USE_CUPS)
#include "chrome/browser/extensions/api/printing/printing_api_handler.h"
#endif  // BUILDFLAG(USE_CUPS)
#endif  // BUILDFLAG(IS_CHROMEOS)

#if BUILDFLAG(IS_CHROMEOS_ASH)
#include "ash/components/arc/arc_prefs.h"
#include "ash/constants/ash_pref_names.h"
#include "ash/public/cpp/ash_prefs.h"
#include "chrome/browser/apps/app_deduplication_service/app_deduplication_service.h"
#include "chrome/browser/apps/app_discovery_service/almanac_fetcher.h"
#include "chrome/browser/apps/app_preload_service/app_preload_service.h"
#include "chrome/browser/apps/app_service/metrics/app_platform_metrics_service.h"
#include "chrome/browser/apps/app_service/webapk/webapk_prefs.h"
#include "chrome/browser/ash/account_manager/account_apps_availability.h"
#include "chrome/browser/ash/account_manager/account_manager_edu_coexistence_controller.h"
#include "chrome/browser/ash/app_list/app_list_syncable_service.h"
#include "chrome/browser/ash/app_list/arc/arc_app_list_prefs.h"
#include "chrome/browser/ash/app_mode/arc/arc_kiosk_app_manager.h"
#include "chrome/browser/ash/app_mode/kiosk_chrome_app_manager.h"
#include "chrome/browser/ash/app_mode/kiosk_cryptohome_remover.h"
#include "chrome/browser/ash/app_mode/kiosk_system_session.h"
#include "chrome/browser/ash/app_mode/web_app/web_kiosk_app_manager.h"
#include "chrome/browser/ash/app_restore/full_restore_prefs.h"
#include "chrome/browser/ash/apps/apk_web_app_service.h"
#include "chrome/browser/ash/arc/boot_phase_monitor/arc_boot_phase_monitor_bridge.h"
#include "chrome/browser/ash/arc/policy/arc_policy_bridge.h"
#include "chrome/browser/ash/arc/session/arc_session_manager.h"
#include "chrome/browser/ash/bluetooth/debug_logs_manager.h"
#include "chrome/browser/ash/bluetooth/hats_bluetooth_revamp_trigger_impl.h"
#include "chrome/browser/ash/borealis/borealis_prefs.h"
#include "chrome/browser/ash/bruschetta/bruschetta_pref_names.h"
#include "chrome/browser/ash/cert_provisioning/cert_provisioning_common.h"
#include "chrome/browser/ash/child_accounts/family_user_chrome_activity_metrics.h"
#include "chrome/browser/ash/child_accounts/family_user_metrics_service.h"
#include "chrome/browser/ash/child_accounts/family_user_session_metrics.h"
#include "chrome/browser/ash/child_accounts/parent_access_code/parent_access_service.h"
#include "chrome/browser/ash/child_accounts/screen_time_controller.h"
#include "chrome/browser/ash/child_accounts/time_limits/app_activity_registry.h"
#include "chrome/browser/ash/child_accounts/time_limits/app_time_controller.h"
#include "chrome/browser/ash/crosapi/browser_util.h"
#include "chrome/browser/ash/crostini/crostini_pref_names.h"
#include "chrome/browser/ash/cryptauth/client_app_metadata_provider_service.h"
#include "chrome/browser/ash/cryptauth/cryptauth_device_id_provider_impl.h"
#include "chrome/browser/ash/customization/customization_document.h"
#include "chrome/browser/ash/file_manager/file_manager_pref_names.h"
#include "chrome/browser/ash/file_manager/file_tasks.h"
#include "chrome/browser/ash/file_system_provider/registry.h"
#include "chrome/browser/ash/first_run/first_run.h"
#include "chrome/browser/ash/floating_workspace/floating_workspace_util.h"
#include "chrome/browser/ash/guest_os/guest_id.h"
#include "chrome/browser/ash/guest_os/guest_os_pref_names.h"
#include "chrome/browser/ash/guest_os/guest_os_terminal.h"
#include "chrome/browser/ash/lock_screen_apps/state_controller.h"
#include "chrome/browser/ash/login/demo_mode/demo_mode_resources_remover.h"
#include "chrome/browser/ash/login/demo_mode/demo_session.h"
#include "chrome/browser/ash/login/demo_mode/demo_setup_controller.h"
#include "chrome/browser/ash/login/quick_unlock/fingerprint_storage.h"
#include "chrome/browser/ash/login/quick_unlock/pin_storage_prefs.h"
#include "chrome/browser/ash/login/quick_unlock/quick_unlock_utils.h"
#include "chrome/browser/ash/login/reporting/login_logout_reporter.h"
#include "chrome/browser/ash/login/saml/saml_profile_prefs.h"
#include "chrome/browser/ash/login/screens/enable_adb_sideloading_screen.h"
#include "chrome/browser/ash/login/screens/reset_screen.h"
#include "chrome/browser/ash/login/security_token_session_controller.h"
#include "chrome/browser/ash/login/session/user_session_manager.h"
#include "chrome/browser/ash/login/signin/signin_error_notifier.h"
#include "chrome/browser/ash/login/signin/token_handle_fetcher.h"
#include "chrome/browser/ash/login/startup_utils.h"
#include "chrome/browser/ash/login/users/avatar/user_image_manager.h"
#include "chrome/browser/ash/login/users/avatar/user_image_prefs.h"
#include "chrome/browser/ash/login/users/avatar/user_image_sync_observer.h"
#include "chrome/browser/ash/login/users/chrome_user_manager_impl.h"
#include "chrome/browser/ash/net/ash_proxy_monitor.h"
#include "chrome/browser/ash/net/network_throttling_observer.h"
#include "chrome/browser/ash/plugin_vm/plugin_vm_pref_names.h"
#include "chrome/browser/ash/policy/core/browser_policy_connector_ash.h"
#include "chrome/browser/ash/policy/core/device_cloud_policy_manager_ash.h"
#include "chrome/browser/ash/policy/core/dm_token_storage.h"
#include "chrome/browser/ash/policy/enrollment/auto_enrollment_client_impl.h"
#include "chrome/browser/ash/policy/enrollment/enrollment_requisition_manager.h"
#include "chrome/browser/ash/policy/external_data/handlers/device_wallpaper_image_external_data_handler.h"
#include "chrome/browser/ash/policy/handlers/adb_sideloading_allowance_mode_policy_handler.h"
#include "chrome/browser/ash/policy/handlers/minimum_version_policy_handler.h"
#include "chrome/browser/ash/policy/handlers/tpm_auto_update_mode_policy_handler.h"
#include "chrome/browser/ash/policy/reporting/app_install_event_log_manager_wrapper.h"
#include "chrome/browser/ash/policy/reporting/arc_app_install_event_logger.h"
#include "chrome/browser/ash/policy/reporting/metrics_reporting/metric_reporting_prefs.h"
#include "chrome/browser/ash/policy/scheduled_task_handler/reboot_notifications_scheduler.h"
#include "chrome/browser/ash/policy/status_collector/device_status_collector.h"
#include "chrome/browser/ash/policy/status_collector/status_collector.h"
#include "chrome/browser/ash/power/auto_screen_brightness/metrics_reporter.h"
#include "chrome/browser/ash/power/power_metrics_reporter.h"
#include "chrome/browser/ash/preferences.h"
#include "chrome/browser/ash/printing/cups_printers_manager.h"
#include "chrome/browser/ash/printing/enterprise_printers_provider.h"
#include "chrome/browser/ash/release_notes/release_notes_storage.h"
#include "chrome/browser/ash/scanning/chrome_scanning_app_delegate.h"
#include "chrome/browser/ash/scheduler_configuration_manager.h"
#include "chrome/browser/ash/settings/device_settings_cache.h"
#include "chrome/browser/ash/system/automatic_reboot_manager.h"
#include "chrome/browser/ash/system/input_device_settings.h"
#include "chrome/browser/ash/system_web_apps/apps/help_app/help_app_notification_controller.h"
#include "chrome/browser/device_identity/chromeos/device_oauth2_token_store_chromeos.h"
#include "chrome/browser/extensions/extension_assets_manager_chromeos.h"
#include "chrome/browser/media/protected_media_identifier_permission_context.h"
#include "chrome/browser/metrics/chromeos_metrics_provider.h"
#include "chrome/browser/ui/ash/shelf/chrome_shelf_prefs.h"
#include "chrome/browser/ui/webui/ash/login/enable_debugging_screen_handler.h"
#include "chrome/browser/ui/webui/ash/settings/os_settings_ui.h"
#include "chrome/browser/ui/webui/settings/reset_settings_handler.h"
#include "chrome/browser/upgrade_detector/upgrade_detector_chromeos.h"
#include "chromeos/ash/components/audio/audio_devices_pref_handler_impl.h"
#include "chromeos/ash/components/local_search_service/search_metrics_reporter.h"
#include "chromeos/ash/components/network/cellular_esim_profile_handler_impl.h"
#include "chromeos/ash/components/network/cellular_metrics_logger.h"
#include "chromeos/ash/components/network/fast_transition_observer.h"
#include "chromeos/ash/components/network/managed_cellular_pref_handler.h"
#include "chromeos/ash/components/network/network_metadata_store.h"
#include "chromeos/ash/components/network/proxy/proxy_config_handler.h"
#include "chromeos/ash/components/report/report_controller.h"
#include "chromeos/ash/components/standalone_browser/migrator_util.h"
#include "chromeos/ash/components/timezone/timezone_resolver.h"
#include "chromeos/ash/services/assistant/public/cpp/assistant_prefs.h"
#include "chromeos/ash/services/auth_factor_config/auth_factor_config.h"
#include "chromeos/ash/services/bluetooth_config/bluetooth_power_controller_impl.h"
#include "chromeos/ash/services/bluetooth_config/device_name_manager_impl.h"
#include "chromeos/ash/services/device_sync/public/cpp/device_sync_prefs.h"
#include "chromeos/ash/services/multidevice_setup/multidevice_setup_service.h"
#include "chromeos/components/quick_answers/public/cpp/quick_answers_prefs.h"
#include "components/account_manager_core/chromeos/account_manager.h"
#include "components/onc/onc_pref_names.h"
#include "components/quirks/quirks_manager.h"
#include "extensions/browser/api/lock_screen_data/lock_screen_item_storage.h"

#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

#if BUILDFLAG(IS_MAC)
#include "chrome/browser/media/webrtc/system_media_capture_permissions_stats_mac.h"
#include "chrome/browser/ui/cocoa/apps/quit_with_apps_controller_mac.h"
#include "chrome/browser/ui/cocoa/confirm_quit.h"
#include "chrome/browser/web_applications/app_shim_registry_mac.h"
#endif

#if BUILDFLAG(IS_WIN)
#include "chrome/browser/enterprise/platform_auth/platform_auth_policy_observer.h"
#include "chrome/browser/font_prewarmer_tab_helper.h"
#include "chrome/browser/media/cdm_pref_service_helper.h"
#include "chrome/browser/media/media_foundation_service_monitor.h"
#include "chrome/browser/os_crypt/app_bound_encryption_metrics_win.h"
#if BUILDFLAG(GOOGLE_CHROME_BRANDING)
#include "chrome/browser/win/conflicts/incompatible_applications_updater.h"
#include "chrome/browser/win/conflicts/module_database.h"
#include "chrome/browser/win/conflicts/third_party_conflicts_manager.h"
#endif  // BUILDFLAG(GOOGLE_CHROME_BRANDING)
#endif  // BUILDFLAG(IS_WIN)

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
#include "components/os_crypt/sync/os_crypt.h"
#endif

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX) || \
    BUILDFLAG(IS_CHROMEOS_ASH)
#include "components/device_signals/core/browser/pref_names.h"  // nogncheck due to crbug.com/1125897
#endif

// TODO(crbug.com/1052397): Revisit the macro expression once build flag switch
// of lacros-chrome is complete.
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || \
    (BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS_LACROS))
#include "chrome/browser/browser_switcher/browser_switcher_prefs.h"
#endif

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
#include "chrome/browser/enterprise/signin/enterprise_signin_prefs.h"
#endif

#if BUILDFLAG(IS_CHROMEOS_LACROS)
#include "chrome/browser/apps/app_service/metrics/website_metrics_service_lacros.h"
#include "chrome/browser/lacros/account_manager/account_cache.h"
#include "chrome/browser/lacros/app_mode/kiosk_session_service_lacros.h"
#include "chrome/browser/lacros/lacros_prefs.h"
#include "chrome/browser/lacros/net/proxy_config_service_lacros.h"
#endif

#if BUILDFLAG(IS_CHROMEOS_LACROS) || BUILDFLAG(ENABLE_DICE_SUPPORT)
#include "chrome/browser/ui/startup/first_run_service.h"
#endif

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_CHROMEOS)
#include "chrome/browser/downgrade/downgrade_prefs.h"
#endif

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_CHROMEOS_ASH)
#include "chrome/browser/device_identity/device_oauth2_token_store_desktop.h"
#include "chrome/browser/ui/startup/default_browser_prompt.h"
#endif

#if defined(TOOLKIT_VIEWS)
#include "chrome/browser/ui/browser_view_prefs.h"
#include "chrome/browser/ui/side_search/side_search_prefs.h"
#endif

#if BUILDFLAG(ENABLE_SESSION_SERVICE)
#include "chrome/browser/sessions/session_data_service.h"
#include "chrome/browser/sessions/session_service_log.h"
#endif

#if BUILDFLAG(IS_LINUX)
#include "ui/color/system_theme.h"
#endif

#if BUILDFLAG(IS_CHROMEOS_ASH)
#include "chrome/browser/ash/wallpaper_handlers/wallpaper_prefs.h"
#endif

#if BUILDFLAG(ENTERPRISE_DATA_CONTROLS)
#include "components/enterprise/data_controls/prefs.h"
#endif

namespace {

// Please keep the list of deprecated prefs in chronological order. i.e. Add to
// the bottom of the list, not here at the top.

// Deprecated 04/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
const char kEasyUnlockHardlockState[] = "easy_unlock.hardlock_state";
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 04/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
const char kBentoBarEnabled[] = "ash.bento_bar.enabled";
const char kUserHasUsedDesksRecently[] = "ash.user_has_used_desks_recently";
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 04/2023.
#if BUILDFLAG(IS_ANDROID)
const char kUserSettingEnabled[] = "offline_prefetch.enabled";
const char kBackoff[] = "offline_prefetch.backoff";
const char kLimitlessPrefetchingEnabledTimePref[] =
    "offline_prefetch.limitless_prefetching_enabled_time";
const char kPrefetchTestingHeaderPref[] =
    "offline_prefetch.testing_header_value";
const char kEnabledByServer[] = "offline_prefetch.enabled_by_server";
const char kNextForbiddenCheckTimePref[] = "offline_prefetch.next_gpb_check";
const char kPrefetchCachedGCMToken[] = "offline_prefetch.gcm_token";
#endif

// Deprecated 04/2023.
const char kTypeSubscribedForInvalidations[] =
    "invalidation.registered_for_invalidation";
const char kActiveRegistrationToken[] =
    "invalidation.active_registration_token";
const char kFCMInvalidationClientIDCache[] = "fcm.invalidation.client_id_cache";

// Deprecated 04/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
const char kConsolidatedConsentTrial[] = "per_user_metrics.trial_group";
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 04/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
const char kOfficeFilesAlwaysMove[] = "filebrowser.office.always_move";
const char kOfficeMoveConfirmationShown[] =
    "filebrowser.office.move_confirmation_shown";
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 04/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
const char kProximityAuthIsChromeOSLoginEnabled[] =
    "proximity_auth.is_chromeos_login_enabled";
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 04/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
const char kEnrollmentIdUploadedOnChromad[] = "chromad.enrollment_id_uploaded";
const char kLastChromadMigrationAttemptTime[] =
    "chromad.last_migration_attempt_time";
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 05/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
const char kSmartLockSigninAllowed[] = "smart_lock_signin.allowed";
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 05/2023.
#if BUILDFLAG(IS_ANDROID)
const char kVideoTutorialsPreferredLocaleKey[] =
    "video_tutorials.perferred_locale";
const char kVideoTutorialsLastUpdatedTimeKey[] =
    "video_tutorials.last_updated_time";
const char kWeeklyStatsReportingTimestamp[] =
    "explore_sites.weekly_stats_reporting_timestamp";
#endif  // BUILDFLAG(IS_ANDROID)

// Deprecated 05/2023.
const char kForceEnablePepperVideoDecoderDevAPI[] =
    "policy.force_enable_pepper_video_decoder_dev_api";

// Deprecated 05/2023.
const char kUseMojoVideoDecoderForPepperAllowed[] =
    "policy.use_mojo_video_decoder_for_pepper_allowed";

// Deprecated 05/2023.
const char kPPAPISharedImagesSwapChainAllowed[] =
    "policy.ppapi_shared_images_swap_chain_allowed";

// Deprecated 05/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
const char kOfficeSetupComplete[] = "filebrowser.office.setup_complete";
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 05/2023.
#if BUILDFLAG(IS_ANDROID)
const char kTimesUPMAuthErrorShown[] = "times_upm_auth_error_shown";
#endif  // BUILDFLAG(IS_ANDROID)

// Deprecated 05/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
const char kSamlPasswordSyncToken[] = "saml.password_sync_token";
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 05/2023.
#if BUILDFLAG(ENABLE_SCREEN_AI_SERVICE)
const char kScreenAIScheduledDeletionTimePrefName[] =
    "accessibility.screen_ai.scheduled_deletion_time";
#endif

// Deprecated 05/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
const char kEventRemappedToRightClick[] =
    "ash.settings.event_remapped_to_right_click";
#endif

// Deprecated 05/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
const char kSupervisedUserSyncId[] = "ManagedUserSyncId";
const char kSupervisedUserManagers[] = "ManagedUserManagers";
const char kSupervisedUserManagerNames[] = "ManagedUserManagerNames";
const char kSupervisedUserManagerDisplayEmails[] =
    "ManagedUserManagerDisplayEmails";
const char kSupervisedUsersFirstRun[] = "LocallyManagedUsersFirstRun";
const char kSupervisedUserPasswordSchema[] = "SupervisedUserPasswordSchema";
const char kSupervisedUserPasswordSalt[] = "SupervisedUserPasswordSalt";
const char kSupervisedUserPasswordRevision[] = "SupervisedUserPasswordRevision";
const char kSupervisedUserNeedPasswordUpdate[] =
    "SupervisedUserNeedPasswordUpdate";
const char kSupervisedUserIncompleteKey[] = "SupervisedUserHasIncompleteKey";
#endif

// Deprecated 06/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
const char kWebAppCalculatorAppErasureFixAppliedPref[] =
    "web_app.calculator_app_erasure_fix_applied";
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 06/2023.
const char kWebAppsExtensionIDs[] = "web_apps.extension_ids";

// Deprecated 06/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
const char kOsSyncPrefsMigrated[] = "sync.os_sync_prefs_migrated";
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 06/2023.
#if !BUILDFLAG(IS_ANDROID)
const char kShouldShowSidePanelBookmarkTab[] =
    "should_show_side_panel_bookmark_tab";
#endif  // !BUILDFLAG(IS_ANDROID)

// Deprecated 06/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
const char kGaiaLastOnlineSignInTime[] = "gaia.last_online_sign_in_time";
const char kSAMLLastGAIASignInTime[] = "saml.last_gaia_sign_in_time";
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 07/2023.
#if !BUILDFLAG(IS_ANDROID)
const char kLegacyHoverCardImagesEnabled[] = "browser.hovercard_images_enabled";
#endif  // !BUILDFLAG(IS_ANDROID)

#if BUILDFLAG(ENABLE_FEED_V2)
const char kVideoPreviewsType[] = "ntp_snippets.video_previews_type";
#endif  // BUILDFLAG(ENABLE_FEED_V2)

// Deprecated 06/2023.
#if BUILDFLAG(IS_ANDROID)
const char kPrefExplicitLanguageAskShown[] =
    "translate_explicit_language_ask_shown";
#endif  // BUILDFLAG(IS_ANDROID)

// Deprecated 07/2023.
#if !BUILDFLAG(IS_CHROMEOS_ASH)
const char kUnifiedConsentMigrationState[] = "unified_consent.migration_state";
#endif  // !BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 07/2023.
const char kPasswordsGroupingInfoRequested[] =
    "password_manager.passwords_grouping_info_requested";

// Deprecated 07/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
const char kPowerMetricsIdleScreenOffCount[] =
    "power.metrics.idle_screen_off_count";
const char kPowerMetricsIdleSuspendCount[] = "power.metrics.idle_suspend_count";
const char kPowerMetricsLidClosedSuspendCount[] =
    "power.metrics.lid_closed_suspend_count";
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 07/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
const char kHatsPrivacyHubBaselineIsSelected[] =
    "hats_privacy_hub_baseline_is_selected";
const char kHatsPrivacyHubBaselineCycleEndTs[] =
    "hats_privacy_hub_baseline_end_timestamp";
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 07/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
const char kClearUserDataDir1Pref[] = "lacros.clear_user_data_dir_1";
#endif

// Deprecated 07/2023.
const char kShutdownNumProcesses[] = "shutdown.num_processes";
const char kShutdownNumProcessesSlow[] = "shutdown.num_processes_slow";
const char kShutdownType[] = "shutdown.type";

// Deprecated 08/2023.
const char kDriveFsBulkPinningMaxQueueSize[] =
    "drivefs.bulk_pinning.max_queue_size";

// Deprecated 09/2023.
const char kPrivacySandboxM1Unrestricted[] = "privacy_sandbox.m1.unrestricted";
#if BUILDFLAG(IS_WIN)
const char kSwReporter[] = "software_reporter";
const char kChromeCleaner[] = "chrome_cleaner";
const char kSettingsResetPrompt[] = "settings_reset_prompt";
#endif
// A boolean specifying whether the new download bubble UI is enabled. If it is
// set to false, the old download shelf UI will be shown instead.
const char kDownloadBubbleEnabled[] = "download_bubble_enabled";

// Deprecated 09/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
const char kGestureEducationNotificationShown[] =
    "ash.gesture_education.notification_shown";

// Note that this very name is used outside ChromeOS Ash, where it isn't
// deprecated.
const char kSyncInitialSyncFeatureSetupCompleteOnAsh[] =
    "sync.has_setup_completed";
#endif

// Deprecated 09/2023.
const char kPrivacySandboxManuallyControlled[] =
    "privacy_sandbox.manually_controlled";

// Deprecated 09/2023.
#if BUILDFLAG(IS_ANDROID)
const char kSettingsMigratedToUPM[] = "profile.settings_migrated_to_upm";
#endif

// Deprecated 10/2023.
const char kSyncRequested[] = "sync.requested";
const char kDownloadLastCompleteTime[] = "download.last_complete_time";

// Deprecated 10/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
const char kLastSuccessfulDomainPref[] = "android_sms.last_successful_domain";
const char kShouldAttemptReenable[] = "android_sms.should_attempt_reenable";
const char kAudioVolumePercent[] = "settings.audio.volume_percent";
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 10/2023.
#if BUILDFLAG(IS_CHROMEOS)
const char kSupportedLinksAppPrefsKey[] = "supported_links_infobar.apps";
#endif  // BUILDFLAG(IS_CHROMEOS)

// Deprecated 10/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
constexpr char kNightLightCachedLatitude[] = "ash.night_light.cached_latitude";
constexpr char kNightLightCachedLongitude[] =
    "ash.night_light.cached_longitude";
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 11/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
constexpr char kUserGeolocationAllowed[] = "ash.user.geolocation_allowed";
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 11/2023.
const char kPrivacySandboxAntiAbuseInitialized[] =
    "privacy_sandbox.anti_abuse_initialized";

// Deprecated 11/2023.
constexpr char kWebRTCAllowLegacyTLSProtocols[] =
    "webrtc.allow_legacy_tls_protocols";

// Deprecated 11/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
constexpr char kSystemTrayExpanded[] = "ash.system_tray.expanded";
#endif

// Deprecated 11/2023.
constexpr char kPasswordChangeSuccessTrackerFlows[] =
    "password_manager.password_change_success_tracker.flows";
constexpr char kPasswordChangeSuccessTrackerVersion[] =
    "password_manager.password_change_success_tracker.version";

// Deprecated 11/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
constexpr char kImageSearchPrivacyNotice[] =
    "ash.launcher.image_search_privacy_notice";
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 11/2023.
constexpr char kWebAndAppActivityEnabledForShopping[] =
    "web_and_app_activity_enabled_for_shopping";

// Deprecated 12/2023.
#if BUILDFLAG(IS_ANDROID)
const char kTemplatesRandomOrder[] = "content_creation.notes.random_order";
#endif

// Deprecated 12/2023.
#if BUILDFLAG(IS_ANDROID)
const char kDesktopSitePeripheralSettingEnabled[] =
    "desktop_site.peripheral_setting";
const char kDesktopSiteDisplaySettingEnabled[] = "desktop_site.display_setting";
#endif

// Deprecated 12/2023.
constexpr char kDownloadDuplicateFilePromptEnabled[] =
    "download_duplicate_file_prompt_enabled";

// Deprecated 12/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
constexpr char kIsolatedWebAppsEnabled[] = "ash.isolated_web_apps_enabled";
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 12/2023.
const char kPrivacyBudgetReportedReidBlocks[] =
    "privacy_budget.reported_reid_blocks";

// Deprecated from profile prefs 12/2023.
const char kModelQualityLoggingClientId[] =
    "optimization_guide.model_quality_logging_client_id";

// Deprecated 12/2023.
const char kSync_ExplicitBrowserSignin[] = "sync.explicit_browser_signin";

// Deprecated 01/2024.
const char kPrivacySandboxPageViewed[] = "privacy_sandbox.page_viewed";

// Deprecated 01/2024.
const char kPrivacySandboxApisEnabledV2[] = "privacy_sandbox.apis_enabled_v2";
const char kPrivacySandboxManuallyControlledV2[] =
    "privacy_sandbox.manually_controlled_v2";

// Deprecated 01/2024.
#if BUILDFLAG(ENABLE_COMPOSE)
constexpr char kPrefHasAcceptedComposeConsent[] =
    "compose_has_accepted_consent";
constexpr char kAutofillAssistanceEnabled[] = "autofill_assistance.enabled";
#endif

// Deprecated 01/2024.
const char kSyncedLastTimePasswordCheckCompleted[] =
    "profile.credentials_last_password_checkup_time";

// Deprecated 01/2024.
const char kDownloadBubbleIphSuppression[] = "suppress_download_bubble_iph";

#if BUILDFLAG(IS_CHROMEOS_ASH)
// Deprecated 01/2024.
const char kPersistedSystemExtensions[] = "system_extensions.persisted";
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 01/2024.
const char kPPAPISharedImagesForVideoDecoderAllowed[] =
    "policy.ppapi_shared_images_for_video_decoder_allowed";

#if BUILDFLAG(IS_CHROMEOS_ASH)
// Deprecated 01/2024.
const char kBorealisVmTokenHash[] = "borealis.vm_token_hash";
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 01/2024.
#if BUILDFLAG(IS_CHROMEOS_ASH)
constexpr char kExtendedFkeysModifier[] =
    "ash.settings.extended_fkeys_modifier";
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 01/2024.
constexpr char kNtpShownPage[] = "ntp.shown_page";
constexpr char kNtpAppPageNames[] = "ntp.app_page_names";

// Deprecated 01/2024.
#if BUILDFLAG(IS_WIN)
const char kSearchResultsPagePrimaryFontsPref[] =
    "cached_fonts.search_results_page.primary";
const char kSearchResultsPageFallbackFontsPref[] =
    "cached_fonts.search_results_page.fallback";
#endif  // BUILDFLAG(IS_WIN)

// Deprecated 01/2024.
#if BUILDFLAG(IS_CHROMEOS_ASH)
constexpr char kUpdateNotificationLastShownMilestone[] =
    "update_notification_last_shown_milestone";
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 02/2024.
#if BUILDFLAG(IS_ANDROID)
constexpr char kSavePasswordsSuspendedByError[] =
    "profile.save_passwords_suspended_by_error";
#endif
constexpr char kSafeBrowsingDeepScanPromptSeen[] =
    "safebrowsing.deep_scan_prompt_seen";
constexpr char kSafeBrowsingEsbEnabledTimestamp[] =
    "safebrowsing.esb_enabled_timestamp";

#if BUILDFLAG(IS_MAC)
constexpr char kScreenTimeEnabled[] = "policy.screen_time";
#endif

// Deprecated 02/2024.
#if BUILDFLAG(IS_CHROMEOS_ASH)
constexpr std::array<const char*, 6u>
    kWelcomeTourTimeBucketsOfFirstInteractions = {
        "ash.welcome_tour.interaction_time.ExploreApp.first_time_bucket",
        "ash.welcome_tour.interaction_time.FilesApp.first_time_bucket",
        "ash.welcome_tour.interaction_time.Launcher.first_time_bucket",
        "ash.welcome_tour.interaction_time.QuickSettings.first_time_bucket",
        "ash.welcome_tour.interaction_time.Search.first_time_bucket",
        "ash.welcome_tour.interaction_time.SettingsApp.first_time_bucket",
};

// Deprecated 02/2024.
constexpr char kDiscoverTabSuggestionChipTimesLeftToShow[] =
    "times_left_to_show_discover_tab_suggestion_chip";
#endif

// Deprecated 02/2024
constexpr char kSearchEnginesChoiceProfile[] = "search_engines.choice_profile";

// Deprecated 02/2024.
#if BUILDFLAG(IS_CHROMEOS_ASH)
constexpr char kHatsUnlockSurveyCycleEndTs[] =
    "hats_unlock_cycle_end_timestamp";
constexpr char kHatsUnlockDeviceIsSelected[] = "hats_unlock_device_is_selected";
constexpr char kHatsSmartLockSurveyCycleEndTs[] =
    "hats_smartlock_cycle_end_timestamp";
constexpr char kHatsSmartLockDeviceIsSelected[] =
    "hats_smartlock_device_is_selected";
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 02/2024
constexpr char kResetCheckDefaultBrowser[] =
    "browser.should_reset_check_default_browser";

#if BUILDFLAG(IS_WIN)
// Deprecated 02/2024
constexpr char kOsCryptAppBoundFixedDataPrefName[] =
    "os_crypt.app_bound_fixed_data";
#endif  // BUILDFLAG(IS_WIN)

// Deprecated 02/2024.
constexpr char kOfferReaderMode[] = "dom_distiller.offer_reader_mode";

// Deprecated 03/2024.
constexpr char kPlusAddressLastFetchedTime[] = "plus_address.last_fetched_time";

// Deprecated 03/2024.
constexpr char kPrivacySandboxApisEnabled[] = "privacy_sandbox.apis_enabled";

#if BUILDFLAG(IS_CHROMEOS_ASH)
// Deprecated 03/2024
constexpr char kOobeGuestAcceptedTos[] = "oobe.guest_accepted_tos";
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 03/2024.
// A `kDefaultSearchProviderChoicePending` pref persists (migrated to a new
// pref name to reset the data), so the variable name has been changed here.
constexpr char kDefaultSearchProviderChoicePendingDeprecated[] =
    "default_search_provider.choice_pending";

// Register local state used only for migration (clearing or moving to a new
// key).
void RegisterLocalStatePrefsForMigration(PrefRegistrySimple* registry) {
// Deprecated 04/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  registry->RegisterDictionaryPref(kEasyUnlockHardlockState);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

  // Deprecated 04/2023.
  registry->RegisterDictionaryPref(kTypeSubscribedForInvalidations);
  registry->RegisterStringPref(kActiveRegistrationToken, std::string());
  registry->RegisterStringPref(kFCMInvalidationClientIDCache, std::string());

// Deprecated 04/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  registry->RegisterStringPref(kConsolidatedConsentTrial, std::string());
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 04/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  registry->RegisterBooleanPref(kEnrollmentIdUploadedOnChromad, false);
  registry->RegisterTimePref(kLastChromadMigrationAttemptTime,
                             /*default_value=*/base::Time());
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

  // Deprecated 05/2023.
  registry->RegisterBooleanPref(kForceEnablePepperVideoDecoderDevAPI, false);

  // Deprecated 05/2023.
  registry->RegisterBooleanPref(kUseMojoVideoDecoderForPepperAllowed, true);

  // Deprecated 05/2023.
  registry->RegisterBooleanPref(kPPAPISharedImagesSwapChainAllowed, true);

// Deprecated 05/2023.
#if BUILDFLAG(ENABLE_SCREEN_AI_SERVICE)
  registry->RegisterTimePref(kScreenAIScheduledDeletionTimePrefName,
                             base::Time());
#endif

// Deprecated 05/2023.
#if BUILDFLAG(IS_ANDROID)
  registry->RegisterTimePref(kWeeklyStatsReportingTimestamp, base::Time());
#endif  // BUILDFLAG(IS_ANDROID)

// Deprecated 05/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  registry->RegisterListPref(kSupervisedUsersFirstRun);
  registry->RegisterDictionaryPref(kSupervisedUserSyncId);
  registry->RegisterDictionaryPref(kSupervisedUserManagers);
  registry->RegisterDictionaryPref(kSupervisedUserManagerNames);
  registry->RegisterDictionaryPref(kSupervisedUserManagerDisplayEmails);

  registry->RegisterDictionaryPref(kSupervisedUserPasswordSchema);
  registry->RegisterDictionaryPref(kSupervisedUserPasswordSalt);
  registry->RegisterDictionaryPref(kSupervisedUserPasswordRevision);

  registry->RegisterDictionaryPref(kSupervisedUserNeedPasswordUpdate);
  registry->RegisterDictionaryPref(kSupervisedUserIncompleteKey);
#endif

// Deprecated 07/2023.
#if !BUILDFLAG(IS_ANDROID)
  registry->RegisterBooleanPref(kLegacyHoverCardImagesEnabled, false);
#endif  // !BUILDFLAG(IS_ANDROID)

  // Deprecated 07/2023.
  registry->RegisterIntegerPref(kShutdownNumProcesses, 0);
  registry->RegisterIntegerPref(kShutdownNumProcessesSlow, 0);
  registry->RegisterIntegerPref(kShutdownType, 0);

  // Deprecated 09/2023.
#if BUILDFLAG(IS_WIN)
  registry->RegisterDictionaryPref(kSwReporter);
  registry->RegisterDictionaryPref(kChromeCleaner);
#endif

  // Deprecated 09/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  registry->RegisterBooleanPref(kGestureEducationNotificationShown, true);
#endif

  // Deprecated 11/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  registry->RegisterBooleanPref(kIsolatedWebAppsEnabled, false);
#endif

  // Deprecated 12/2023.
  registry->RegisterStringPref(kPrivacyBudgetReportedReidBlocks, std::string());

  // Deprecated 01/2024.
  registry->RegisterBooleanPref(kPPAPISharedImagesForVideoDecoderAllowed, true);

  // Deprecated 01/2024.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  registry->RegisterIntegerPref(kExtendedFkeysModifier, 0);
#endif

  // Deprecated 02/2024
#if BUILDFLAG(IS_MAC)
  registry->RegisterBooleanPref(kScreenTimeEnabled, true);
#endif

  // Deprecated 02/2024.
  registry->RegisterFilePathPref(kSearchEnginesChoiceProfile, base::FilePath());

#if BUILDFLAG(IS_WIN)
  // Deprecated 02/2024.
  registry->RegisterStringPref(kOsCryptAppBoundFixedDataPrefName,
                               std::string());
#endif

#if BUILDFLAG(IS_CHROMEOS_ASH)
  // Deprecated 03/2024.
  registry->RegisterBooleanPref(kOobeGuestAcceptedTos, false);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)
}

// Register prefs used only for migration (clearing or moving to a new key).
void RegisterProfilePrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry) {
  chrome_browser_net::secure_dns::RegisterProbesSettingBackupPref(registry);

// Deprecated 04/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  registry->RegisterBooleanPref(kBentoBarEnabled, false);
  registry->RegisterBooleanPref(kUserHasUsedDesksRecently, false);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 04/2023.
#if BUILDFLAG(IS_ANDROID)
  registry->RegisterListPref(kBackoff);
  registry->RegisterBooleanPref(kUserSettingEnabled, true);
  registry->RegisterTimePref(kLimitlessPrefetchingEnabledTimePref,
                             base::Time());
  registry->RegisterStringPref(kPrefetchTestingHeaderPref, std::string());
  registry->RegisterBooleanPref(kEnabledByServer, false);
  registry->RegisterTimePref(kNextForbiddenCheckTimePref, base::Time());
  registry->RegisterStringPref(kPrefetchCachedGCMToken, std::string());
#endif

  // Deprecated 04/2023.
  registry->RegisterDictionaryPref(kTypeSubscribedForInvalidations);
  registry->RegisterStringPref(kActiveRegistrationToken, std::string());
  registry->RegisterStringPref(kFCMInvalidationClientIDCache, std::string());

  // Deprecated 04/2023.
#if BUILDFLAG(IS_ANDROID)
  ntp_snippets::prefs::RegisterProfilePrefsForMigrationApril2023(registry);
#endif

// Deprecated 04/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  registry->RegisterBooleanPref(kOfficeFilesAlwaysMove, false);
  registry->RegisterBooleanPref(kOfficeMoveConfirmationShown, false);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 04/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  registry->RegisterBooleanPref(kProximityAuthIsChromeOSLoginEnabled, false);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 05/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  registry->RegisterBooleanPref(kSmartLockSigninAllowed, false);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

  // Deprecated 05/2023.
#if BUILDFLAG(IS_ANDROID)
  registry->RegisterStringPref(kVideoTutorialsPreferredLocaleKey,
                               std::string());
  registry->RegisterTimePref(kVideoTutorialsLastUpdatedTimeKey, base::Time());
#endif  // BUILDFLAG(IS_ANDROID)

// Deprecated 05/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  registry->RegisterBooleanPref(kOfficeSetupComplete, false);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 05/2023.
#if BUILDFLAG(IS_ANDROID)
  registry->RegisterIntegerPref(kTimesUPMAuthErrorShown, 0);
#endif  // BUILDFLAG(IS_ANDROID)

// Deprecated 05/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  registry->RegisterStringPref(kSamlPasswordSyncToken, std::string());
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 05/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  registry->RegisterBooleanPref(kEventRemappedToRightClick, false);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 06/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  registry->RegisterBooleanPref(kWebAppCalculatorAppErasureFixAppliedPref,
                                false);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

  // Deprecated 06/2023.
  registry->RegisterDictionaryPref(kWebAppsExtensionIDs);

  // Deprecated 06/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  registry->RegisterBooleanPref(kOsSyncPrefsMigrated, false);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

  // Deprecated 06/2023.
#if !BUILDFLAG(IS_ANDROID)
  registry->RegisterBooleanPref(kShouldShowSidePanelBookmarkTab, false);
#endif  // !BUILDFLAG(IS_ANDROID)

  // Deprecated 06/2023.
#if BUILDFLAG(ENABLE_FEED_V2)
  registry->RegisterIntegerPref(kVideoPreviewsType, 1);
#endif  // BUILDFLAG(ENABLE_FEED_V2)

// Deprecated 06/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  registry->RegisterTimePref(kGaiaLastOnlineSignInTime, base::Time());
  registry->RegisterTimePref(kSAMLLastGAIASignInTime, base::Time());
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

  // Deprecated 06/2023.
#if BUILDFLAG(IS_ANDROID)
  registry->RegisterBooleanPref(kPrefExplicitLanguageAskShown, false);
#endif  // BUILDFLAG(IS_ANDROID)

// Deprecated 07/2023.
#if !BUILDFLAG(IS_CHROMEOS_ASH)
  registry->RegisterIntegerPref(kUnifiedConsentMigrationState, 0);
#endif  // !BUILDFLAG(IS_CHROMEOS_ASH)

  // Deprecated 07/2023.
  registry->RegisterBooleanPref(kPasswordsGroupingInfoRequested, false);

// Deprecated 07/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  registry->RegisterIntegerPref(kPowerMetricsIdleScreenOffCount, 0);
  registry->RegisterIntegerPref(kPowerMetricsIdleSuspendCount, 0);
  registry->RegisterIntegerPref(kPowerMetricsLidClosedSuspendCount, 0);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 07/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  registry->RegisterIntegerPref(kHatsPrivacyHubBaselineIsSelected, false);
  registry->RegisterIntegerPref(kHatsPrivacyHubBaselineCycleEndTs, 0);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

  // Deprecated 08/2023.
  registry->RegisterIntegerPref(kDriveFsBulkPinningMaxQueueSize, 0);

  // Deprecated 09/2023.
  registry->RegisterBooleanPref(kPrivacySandboxM1Unrestricted, false);
#if BUILDFLAG(IS_WIN)
  registry->RegisterDictionaryPref(kSwReporter);
  registry->RegisterDictionaryPref(kSettingsResetPrompt);
  registry->RegisterDictionaryPref(kChromeCleaner);
#endif
  registry->RegisterBooleanPref(kDownloadBubbleEnabled, true);
  registry->RegisterBooleanPref(kPrivacySandboxManuallyControlled, false);
#if BUILDFLAG(IS_CHROMEOS_ASH)
  registry->RegisterBooleanPref(kSyncInitialSyncFeatureSetupCompleteOnAsh,
                                false);
#endif
#if BUILDFLAG(IS_ANDROID)
  registry->RegisterBooleanPref(kSettingsMigratedToUPM, false);
#endif
  registry->RegisterBooleanPref(kSyncRequested, false);

  // Deprecated 10/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  registry->RegisterStringPref(kLastSuccessfulDomainPref, std::string());
  registry->RegisterBooleanPref(kShouldAttemptReenable, true);
  registry->RegisterDoublePref(kAudioVolumePercent, 0);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)
  registry->RegisterTimePref(kDownloadLastCompleteTime, base::Time());

// Deprecated 10/2023.
#if BUILDFLAG(IS_CHROMEOS)
  registry->RegisterDictionaryPref(kSupportedLinksAppPrefsKey);
#endif  // BUILDFLAG(IS_CHROMEOS)

// Deprecated 10/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  registry->RegisterDoublePref(kNightLightCachedLatitude, 0.0);
  registry->RegisterDoublePref(kNightLightCachedLongitude, 0.0);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

  // Deprecated 11/2023.
  registry->RegisterBooleanPref(kPrivacySandboxAntiAbuseInitialized, false);

  // Deprecated 11/2023.
  registry->RegisterBooleanPref(kWebRTCAllowLegacyTLSProtocols, false);

// Deprecated 11/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  registry->RegisterBooleanPref(kSystemTrayExpanded, true);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Deprecated 11/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  registry->RegisterBooleanPref(kUserGeolocationAllowed, true);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

  // Deprecated 11/2023.
  registry->RegisterListPref(kPasswordChangeSuccessTrackerFlows);
  registry->RegisterIntegerPref(kPasswordChangeSuccessTrackerVersion, 0);

  // Deprecated 11/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  registry->RegisterDictionaryPref(kImageSearchPrivacyNotice);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

  // Deprecated 11/2023.
  registry->RegisterBooleanPref(kWebAndAppActivityEnabledForShopping, true);

  // Deprecated 12/2023.
#if BUILDFLAG(IS_ANDROID)
  registry->RegisterListPref(kTemplatesRandomOrder);
#endif

// Deprecated 12/2023.
#if BUILDFLAG(IS_ANDROID)
  registry->RegisterBooleanPref(kDesktopSitePeripheralSettingEnabled, false);
  registry->RegisterBooleanPref(kDesktopSiteDisplaySettingEnabled, false);
#endif

  // Deprecated 12/2023.
  registry->RegisterBooleanPref(kDownloadDuplicateFilePromptEnabled, true);

  // Deprecated 12/2023.
  registry->RegisterInt64Pref(kModelQualityLoggingClientId, true);
  registry->RegisterBooleanPref(kSync_ExplicitBrowserSignin, false);

  // Deprecated 01/2024.
  registry->RegisterBooleanPref(kPrivacySandboxPageViewed, false);

  // Deprecated 01/2024.
  registry->RegisterBooleanPref(kPrivacySandboxApisEnabledV2, false);
  registry->RegisterBooleanPref(kPrivacySandboxManuallyControlledV2, false);

// Deprecated 01/2024.
#if BUILDFLAG(ENABLE_COMPOSE)
  registry->RegisterBooleanPref(kPrefHasAcceptedComposeConsent, false);
  registry->RegisterBooleanPref(kAutofillAssistanceEnabled, false);
#endif

  // Deprecated 01/2024.
  registry->RegisterTimePref(kSyncedLastTimePasswordCheckCompleted,
                             base::Time());

  // Deprecated 01/2024.
  registry->RegisterBooleanPref(kDownloadBubbleIphSuppression, false);

// Deprecated 01/2024.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  registry->RegisterDictionaryPref(kPersistedSystemExtensions);
  registry->RegisterStringPref(kBorealisVmTokenHash, "");
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

  // Deprecated 01/2024.
  registry->RegisterIntegerPref(kNtpShownPage, 0);
  registry->RegisterListPref(kNtpAppPageNames);

  // Deprecated 01/2024.
#if BUILDFLAG(IS_WIN)
  registry->RegisterListPref(kSearchResultsPagePrimaryFontsPref);
  registry->RegisterListPref(kSearchResultsPageFallbackFontsPref);
#endif

  // Deprecated 01/2024.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  registry->RegisterIntegerPref(kUpdateNotificationLastShownMilestone, -10);
#endif

  // Deprecated 02/2024.
#if BUILDFLAG(IS_ANDROID)
  registry->RegisterBooleanPref(kSavePasswordsSuspendedByError, false);
#endif
  registry->RegisterBooleanPref(kSafeBrowsingDeepScanPromptSeen, false);
  registry->RegisterTimePref(kSafeBrowsingEsbEnabledTimestamp, base::Time());

// Deprecated 02/2024.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  for (const char* pref : kWelcomeTourTimeBucketsOfFirstInteractions) {
    registry->RegisterIntegerPref(pref, -1);
  }

  // Deprecated 02/2024.
  registry->RegisterIntegerPref(kDiscoverTabSuggestionChipTimesLeftToShow, 0);
#endif

  // Deprecated 02/2024.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  registry->RegisterInt64Pref(kHatsSmartLockSurveyCycleEndTs, 0);
  registry->RegisterBooleanPref(kHatsSmartLockDeviceIsSelected, false);
  registry->RegisterInt64Pref(kHatsUnlockSurveyCycleEndTs, 0);
  registry->RegisterBooleanPref(kHatsUnlockDeviceIsSelected, false);
#endif

  // Deprecated 02/2024
  registry->RegisterBooleanPref(kResetCheckDefaultBrowser, false);

  // Deprecated 02/2024.
  registry->RegisterBooleanPref(kOfferReaderMode, false);

  // Deprecated 03/2024.
  registry->RegisterTimePref(kPlusAddressLastFetchedTime, base::Time());

  // Deprecated 03/2024.
  registry->RegisterBooleanPref(kPrivacySandboxApisEnabled, true);

  // Deprecated 03/2024.
  registry->RegisterBooleanPref(kDefaultSearchProviderChoicePendingDeprecated,
                                false);
}

void ClearSyncRequestedPrefAndMaybeMigrate(PrefService* profile_prefs) {
#if BUILDFLAG(IS_CHROMEOS_ASH)
  // On Ash specifically, if `kSyncRequested` was set to false explicitly, the
  // value needs to be migrated to syncer::internal::kSyncDisabledViaDashboard.
  if (profile_prefs->GetUserPrefValue(kSyncRequested) != nullptr &&
      !profile_prefs->GetUserPrefValue(kSyncRequested)->GetBool()) {
    profile_prefs->SetBoolean(
        syncer::prefs::internal::kSyncDisabledViaDashboard, true);
  }
#endif
  profile_prefs->ClearPref(kSyncRequested);
}

}  // namespace

std::string GetCountry() {
  if (!g_browser_process || !g_browser_process->variations_service()) {
    // This should only happen in tests. Ideally this would be guarded by
    // CHECK_IS_TEST, but that is not set on Android, so no specific guard.
    return std::string();
  }
  return std::string(
      g_browser_process->variations_service()->GetStoredPermanentCountry());
}

void RegisterLocalState(PrefRegistrySimple* registry) {
  // Call outs to individual subsystems that register Local State (browser-wide)
  // prefs en masse. See RegisterProfilePrefs for per-profile prefs. Please
  // keep this list alphabetized.
#if BUILDFLAG(IS_ANDROID)
  accessibility::AccessibilityPrefsController::RegisterLocalStatePrefs(
      registry);
#endif
  breadcrumbs::RegisterPrefs(registry);
  browser_shutdown::RegisterPrefs(registry);
  BrowserProcessImpl::RegisterPrefs(registry);
  ChromeContentBrowserClient::RegisterLocalStatePrefs(registry);
  chrome_labs_prefs::RegisterLocalStatePrefs(registry);
  ChromeMetricsServiceClient::RegisterPrefs(registry);
  chrome::enterprise_util::RegisterLocalStatePrefs(registry);
  component_updater::RegisterPrefs(registry);
  domain_reliability::RegisterPrefs(registry);
  embedder_support::OriginTrialPrefs::RegisterPrefs(registry);
  enterprise_reporting::RegisterLocalStatePrefs(registry);
  ExternalProtocolHandler::RegisterPrefs(registry);
  flags_ui::PrefServiceFlagsStorage::RegisterPrefs(registry);
  GpuModeManager::RegisterPrefs(registry);
  signin::IdentityManager::RegisterLocalStatePrefs(registry);
  invalidation::FCMInvalidationService::RegisterPrefs(registry);
  invalidation::InvalidatorRegistrarWithMemory::RegisterPrefs(registry);
  invalidation::PerUserTopicSubscriptionManager::RegisterPrefs(registry);
  language::GeoLanguageProvider::RegisterLocalStatePrefs(registry);
  language::UlpLanguageCodeLocator::RegisterLocalStatePrefs(registry);
  memory::EnterpriseMemoryLimitPrefObserver::RegisterPrefs(registry);
  metrics::RegisterDemographicsLocalStatePrefs(registry);
  network_time::NetworkTimeTracker::RegisterPrefs(registry);
  optimization_guide::prefs::RegisterLocalStatePrefs(registry);
  password_manager::PasswordManager::RegisterLocalPrefs(registry);
  policy::BrowserPolicyConnector::RegisterPrefs(registry);
  policy::LocalTestPolicyProvider::RegisterLocalStatePrefs(registry);
  policy::ManagementService::RegisterLocalStatePrefs(registry);
  policy::PolicyStatisticsCollector::RegisterPrefs(registry);
  PrefProxyConfigTrackerImpl::RegisterPrefs(registry);
  ProfileAttributesEntry::RegisterLocalStatePrefs(registry);
  ProfileAttributesStorage::RegisterPrefs(registry);
  ProfileNetworkContextService::RegisterLocalStatePrefs(registry);
  profiles::RegisterPrefs(registry);
#if BUILDFLAG(IS_ANDROID)
  PushMessagingServiceImpl::RegisterPrefs(registry);
#endif
  RegisterScreenshotPrefs(registry);
  safe_browsing::RegisterLocalStatePrefs(registry);
  secure_origin_allowlist::RegisterPrefs(registry);
  segmentation_platform::SegmentationPlatformService::RegisterLocalStatePrefs(
      registry);
#if !BUILDFLAG(IS_ANDROID)
  SerialPolicyAllowedPorts::RegisterPrefs(registry);
  HidPolicyAllowedDevices::RegisterLocalStatePrefs(registry);
#endif
  sessions::SessionIdGenerator::RegisterPrefs(registry);
  SSLConfigServiceManager::RegisterPrefs(registry);
  subresource_filter::IndexedRulesetVersion::RegisterPrefs(
      registry, subresource_filter::kSafeBrowsingFilterTag);
  SystemNetworkContextManager::RegisterPrefs(registry);
  tpcd::experiment::RegisterLocalStatePrefs(registry);
  tracing::RegisterPrefs(registry);
  update_client::RegisterPrefs(registry);
  variations::VariationsService::RegisterPrefs(registry);

  // Individual preferences. If you have multiple preferences that should
  // clearly be grouped together, please group them together into a helper
  // function called above. Please keep this list alphabetized.
  registry->RegisterBooleanPref(
      policy::policy_prefs::kIntensiveWakeUpThrottlingEnabled, false);
  registry->RegisterBooleanPref(
      policy::policy_prefs::kUserAgentClientHintsGREASEUpdateEnabled, true);
#if BUILDFLAG(IS_ANDROID)
  registry->RegisterBooleanPref(policy::policy_prefs::kBackForwardCacheEnabled,
                                true);
  registry->RegisterBooleanPref(policy::policy_prefs::kReadAloudEnabled, true);
#endif  // BUILDFLAG(IS_ANDROID)

  // Below this point is for platform-specific and compile-time conditional
  // calls. Please follow the helper-function-first-then-direct-calls pattern
  // established above, and keep things alphabetized.

#if BUILDFLAG(ENABLE_BACKGROUND_MODE)
  BackgroundModeManager::RegisterPrefs(registry);
#endif

#if BUILDFLAG(IS_ANDROID)
  ::android::RegisterPrefs(registry);

  registry->RegisterIntegerPref(first_run::kTosDialogBehavior, 0);
  registry->RegisterBooleanPref(lens::kLensCameraAssistedSearchEnabled, true);
#else   // BUILDFLAG(IS_ANDROID)
  enterprise_connectors::RegisterLocalStatePrefs(registry);
  gcm::RegisterPrefs(registry);
  headless::RegisterPrefs(registry);
  IntranetRedirectDetector::RegisterPrefs(registry);
  media_router::RegisterLocalStatePrefs(registry);
  metrics::TabStatsTracker::RegisterPrefs(registry);
  performance_manager::user_tuning::prefs::RegisterLocalStatePrefs(registry);
  RegisterBrowserPrefs(registry);
  SearchEngineChoiceClientSideTrial::RegisterLocalStatePrefs(registry);
  speech::SodaInstaller::RegisterLocalStatePrefs(registry);
  StartupBrowserCreator::RegisterLocalStatePrefs(registry);
  task_manager::TaskManagerInterface::RegisterPrefs(registry);
  UpgradeDetector::RegisterPrefs(registry);
  WhatsNewUI::RegisterLocalStatePrefs(registry);
#endif  // BUILDFLAG(IS_ANDROID)

#if BUILDFLAG(IS_CHROMEOS_LACROS) || BUILDFLAG(ENABLE_DICE_SUPPORT)
  FirstRunService::RegisterLocalStatePrefs(registry);
#endif

#if BUILDFLAG(IS_CHROMEOS_ASH)
  arc::prefs::RegisterLocalStatePrefs(registry);
  ChromeOSMetricsProvider::RegisterPrefs(registry);
  ash::ArcKioskAppManager::RegisterPrefs(registry);
  ash::AudioDevicesPrefHandlerImpl::RegisterPrefs(registry);
  ash::carrier_lock::CarrierLockManager::RegisterLocalPrefs(registry);
  ash::cert_provisioning::RegisterLocalStatePrefs(registry);
  ash::CellularESimProfileHandlerImpl::RegisterLocalStatePrefs(registry);
  ash::ManagedCellularPrefHandler::RegisterLocalStatePrefs(registry);
  ash::ChromeUserManagerImpl::RegisterPrefs(registry);
  crosapi::browser_util::RegisterLocalStatePrefs(registry);
  ash::CupsPrintersManager::RegisterLocalStatePrefs(registry);
  ash::BrowserDataMigratorImpl::RegisterLocalStatePrefs(registry);
  ash::bluetooth_config::BluetoothPowerControllerImpl::RegisterLocalStatePrefs(
      registry);
  ash::bluetooth_config::DeviceNameManagerImpl::RegisterLocalStatePrefs(
      registry);
  ash::DemoModeResourcesRemover::RegisterLocalStatePrefs(registry);
  ash::DemoSession::RegisterLocalStatePrefs(registry);
  ash::DemoSetupController::RegisterLocalStatePrefs(registry);
  ash::DeviceNameStore::RegisterLocalStatePrefs(registry);
  chromeos::DeviceOAuth2TokenStoreChromeOS::RegisterPrefs(registry);
  ash::device_settings_cache::RegisterPrefs(registry);
  ash::EnableAdbSideloadingScreen::RegisterPrefs(registry);
  ash::report::ReportController::RegisterPrefs(registry);
  ash::EnableDebuggingScreenHandler::RegisterPrefs(registry);
  ash::FastTransitionObserver::RegisterPrefs(registry);
  ash::HWDataUsageController::RegisterLocalStatePrefs(registry);
  ash::KerberosCredentialsManager::RegisterLocalStatePrefs(registry);
  ash::KioskChromeAppManager::RegisterLocalStatePrefs(registry);
  ash::KioskSystemSession::RegisterLocalStatePrefs(registry);
  ash::KioskCryptohomeRemover::RegisterPrefs(registry);
  ash::language_prefs::RegisterPrefs(registry);
  ash::local_search_service::SearchMetricsReporter::RegisterLocalStatePrefs(
      registry);
  ash::login::SecurityTokenSessionController::RegisterLocalStatePrefs(registry);
  ash::reporting::LoginLogoutReporter::RegisterPrefs(registry);
  ash::reporting::RegisterLocalStatePrefs(registry);
  ash::NetworkMetadataStore::RegisterPrefs(registry);
  ash::NetworkThrottlingObserver::RegisterPrefs(registry);
  ash::PowerMetricsReporter::RegisterLocalStatePrefs(registry);
  ash::platform_keys::KeyPermissionsManagerImpl::RegisterLocalStatePrefs(
      registry);
  ash::power::auto_screen_brightness::MetricsReporter::RegisterLocalStatePrefs(
      registry);
  ash::Preferences::RegisterPrefs(registry);
  ash::ResetScreen::RegisterPrefs(registry);
  settings::ResetSettingsHandler::RegisterLocalStatePrefs(registry);
  ash::SchedulerConfigurationManager::RegisterLocalStatePrefs(registry);
  ash::ServicesCustomizationDocument::RegisterPrefs(registry);
  ash::standalone_browser::migrator_util::RegisterLocalStatePrefs(registry);
  ash::StartupUtils::RegisterPrefs(registry);
  ash::StatsReportingController::RegisterLocalStatePrefs(registry);
  ash::system::AutomaticRebootManager::RegisterPrefs(registry);
  ash::TimeZoneResolver::RegisterPrefs(registry);
  ash::UserImageManager::RegisterPrefs(registry);
  ash::UserSessionManager::RegisterPrefs(registry);
  ash::WebKioskAppManager::RegisterPrefs(registry);
  component_updater::MetadataTable::RegisterPrefs(registry);
  ash::CryptAuthDeviceIdProviderImpl::RegisterLocalPrefs(registry);
  extensions::ExtensionAssetsManagerChromeOS::RegisterPrefs(registry);
  extensions::ExtensionsPermissionsTracker::RegisterLocalStatePrefs(registry);
  extensions::lock_screen_data::LockScreenItemStorage::RegisterLocalState(
      registry);
  extensions::login_api::RegisterLocalStatePrefs(registry);
  ::onc::RegisterPrefs(registry);
  policy::AdbSideloadingAllowanceModePolicyHandler::RegisterPrefs(registry);
  // TODO(b/265923216): Replace with EnrollmentStateFetcher::RegisterPrefs.
  policy::AutoEnrollmentClientImpl::RegisterPrefs(registry);
  policy::BrowserPolicyConnectorAsh::RegisterPrefs(registry);
  policy::CrdAdminSessionController::RegisterLocalStatePrefs(registry);
  policy::DeviceCloudPolicyManagerAsh::RegisterPrefs(registry);
  policy::DeviceStatusCollector::RegisterPrefs(registry);
  policy::DeviceWallpaperImageExternalDataHandler::RegisterPrefs(registry);
  policy::DMTokenStorage::RegisterPrefs(registry);
  policy::EnrollmentRequisitionManager::RegisterPrefs(registry);
  policy::MinimumVersionPolicyHandler::RegisterPrefs(registry);
  policy::EuiccStatusUploader::RegisterLocalStatePrefs(registry);
  policy::TPMAutoUpdateModePolicyHandler::RegisterPrefs(registry);
  quirks::QuirksManager::RegisterPrefs(registry);
  UpgradeDetectorChromeos::RegisterPrefs(registry);
  RegisterNearbySharingLocalPrefs(registry);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

#if BUILDFLAG(IS_CHROMEOS)
  chromeos::echo_offer::RegisterPrefs(registry);
  memory::OOMKillsMonitor::RegisterPrefs(registry);
  policy::SystemFeaturesDisableListPolicyHandler::RegisterPrefs(registry);
  policy::DlpRulesManagerImpl::RegisterPrefs(registry);
#endif  // BUILDFLAG(IS_CHROMEOS)

#if BUILDFLAG(IS_MAC)
  confirm_quit::RegisterLocalState(registry);
  QuitWithAppsController::RegisterPrefs(registry);
  system_media_permissions::RegisterSystemMediaPermissionStatesPrefs(registry);
  AppShimRegistry::Get()->RegisterLocalPrefs(registry);
#endif

#if BUILDFLAG(IS_CHROMEOS_LACROS)
  AccountCache::RegisterLocalStatePrefs(registry);
  lacros_prefs::RegisterLocalStatePrefs(registry);
  KioskSessionServiceLacros::RegisterLocalStatePrefs(registry);
#endif

#if BUILDFLAG(IS_WIN)
  OSCrypt::RegisterLocalPrefs(registry);
  registry->RegisterBooleanPref(prefs::kRendererCodeIntegrityEnabled, true);
  registry->RegisterBooleanPref(prefs::kRendererAppContainerEnabled, true);
  registry->RegisterBooleanPref(prefs::kBlockBrowserLegacyExtensionPoints,
                                true);
  registry->RegisterBooleanPref(
      policy::policy_prefs::kNativeWindowOcclusionEnabled, true);
  MediaFoundationServiceMonitor::RegisterPrefs(registry);
  os_crypt::RegisterLocalStatePrefs(registry);
#if BUILDFLAG(GOOGLE_CHROME_BRANDING)
  IncompatibleApplicationsUpdater::RegisterLocalStatePrefs(registry);
  ModuleDatabase::RegisterLocalStatePrefs(registry);
  ThirdPartyConflictsManager::RegisterLocalStatePrefs(registry);
#endif  // BUILDFLAG(GOOGLE_CHROME_BRANDING)
#endif  // BUILDFLAG(IS_WIN)

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_CHROMEOS)
  downgrade::RegisterPrefs(registry);
#endif

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_CHROMEOS_ASH)
  RegisterDefaultBrowserPromptPrefs(registry);
  DeviceOAuth2TokenStoreDesktop::RegisterPrefs(registry);
#endif

#if BUILDFLAG(ENABLE_SCREEN_AI_SERVICE)
  screen_ai::RegisterLocalStatePrefs(registry);
#endif  // BUILDFLAG(ENABLE_SCREEN_AI_SERVICE)

#if BUILDFLAG(IS_WIN)
  PlatformAuthPolicyObserver::RegisterPrefs(registry);
#endif  // BUILDFLAG(IS_WIN)

  // Platform-specific and compile-time conditional individual preferences.
  // If you have multiple preferences that should clearly be grouped together,
  // please group them together into a helper function called above. Please
  // keep this list alphabetized.

#if BUILDFLAG(ENABLE_OOP_PRINTING)
  registry->RegisterBooleanPref(prefs::kOopPrintDriversAllowedByPolicy, true);
#endif

#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_WIN)
  registry->RegisterBooleanPref(prefs::kChromeForTestingAllowed, true);
#endif

  // This is intentionally last.
  RegisterLocalStatePrefsForMigration(registry);
}

// Register prefs applicable to all profiles.
void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry,
                          const std::string& locale) {
  TRACE_EVENT0("browser", "chrome::RegisterProfilePrefs");
  // User prefs. Please keep this list alphabetized.
  AccessibilityLabelsService::RegisterProfilePrefs(registry);
  AccessibilityUIMessageHandler::RegisterProfilePrefs(registry);
  AnnouncementNotificationService::RegisterProfilePrefs(registry);
  autofill::prefs::RegisterProfilePrefs(registry);
  browsing_data::prefs::RegisterBrowserUserPrefs(registry);
  capture_policy::RegisterProfilePrefs(registry);
  certificate_transparency::prefs::RegisterPrefs(registry);
  ChromeContentBrowserClient::RegisterProfilePrefs(registry);
  chrome_labs_prefs::RegisterProfilePrefs(registry);
  ChromeLocationBarModelDelegate::RegisterProfilePrefs(registry);
  StatefulSSLHostStateDelegate::RegisterProfilePrefs(registry);
  ChromeVersionService::RegisterProfilePrefs(registry);
  chrome_browser_net::NetErrorTabHelper::RegisterProfilePrefs(registry);
  chrome_prefs::RegisterProfilePrefs(registry);
  commerce::RegisterPrefs(registry);
  DocumentProvider::RegisterProfilePrefs(registry);
  enterprise::RegisterIdentifiersProfilePrefs(registry);
  enterprise_reporting::RegisterProfilePrefs(registry);
  dom_distiller::DistilledPagePrefs::RegisterProfilePrefs(registry);
  DownloadPrefs::RegisterProfilePrefs(registry);
  permissions::PermissionHatsTriggerHelper::RegisterProfilePrefs(registry);
  history_clusters::prefs::RegisterProfilePrefs(registry);
  HostContentSettingsMap::RegisterProfilePrefs(registry);
  image_fetcher::ImageCache::RegisterProfilePrefs(registry);
  site_engagement::ImportantSitesUtil::RegisterProfilePrefs(registry);
  IncognitoModePrefs::RegisterProfilePrefs(registry);
  invalidation::PerUserTopicSubscriptionManager::RegisterProfilePrefs(registry);
  invalidation::InvalidatorRegistrarWithMemory::RegisterProfilePrefs(registry);
  language::LanguagePrefs::RegisterProfilePrefs(registry);
  login_detection::prefs::RegisterProfilePrefs(registry);
  lookalikes::RegisterProfilePrefs(registry);
  media_prefs::RegisterUserPrefs(registry);
  MediaCaptureDevicesDispatcher::RegisterProfilePrefs(registry);
  media_device_salt::MediaDeviceIDSalt::RegisterProfilePrefs(registry);
  MediaEngagementService::RegisterProfilePrefs(registry);
  MediaStorageIdSalt::RegisterProfilePrefs(registry);
  metrics::RegisterDemographicsProfilePrefs(registry);
  NotificationDisplayServiceImpl::RegisterProfilePrefs(registry);
  NotifierStateTracker::RegisterProfilePrefs(registry);
  ntp_tiles::MostVisitedSites::RegisterProfilePrefs(registry);
  optimization_guide::prefs::RegisterProfilePrefs(registry);
  optimization_guide::model_execution::prefs::RegisterProfilePrefs(registry);
  password_manager::PasswordManager::RegisterProfilePrefs(registry);
  payments::RegisterProfilePrefs(registry);
  performance_manager::user_tuning::prefs::RegisterProfilePrefs(registry);
  permissions::RegisterProfilePrefs(registry);
  PermissionBubbleMediaAccessHandler::RegisterProfilePrefs(registry);
  PlatformNotificationServiceImpl::RegisterProfilePrefs(registry);
  plus_addresses::RegisterProfilePrefs(registry);
  policy::URLBlocklistManager::RegisterProfilePrefs(registry);
  PolicyUI::RegisterProfilePrefs(registry);
  PrefProxyConfigTrackerImpl::RegisterProfilePrefs(registry);
  prefetch::RegisterPredictionOptionsProfilePrefs(registry);
  PrefetchOriginDecider::RegisterPrefs(registry);
  PrefsTabHelper::RegisterProfilePrefs(registry, locale);
  privacy_sandbox::RegisterProfilePrefs(registry);
  Profile::RegisterProfilePrefs(registry);
  ProfileImpl::RegisterProfilePrefs(registry);
  ProfileNetworkContextService::RegisterProfilePrefs(registry);
  custom_handlers::ProtocolHandlerRegistry::RegisterProfilePrefs(registry);
  PushMessagingAppIdentifier::RegisterProfilePrefs(registry);
  QuietNotificationPermissionUiState::RegisterProfilePrefs(registry);
  RegisterBrowserUserPrefs(registry);
  RegisterPrefersDefaultScrollbarStylesPrefs(registry);
  safe_browsing::file_type::RegisterProfilePrefs(registry);
  safe_browsing::RegisterProfilePrefs(registry);
  SearchPrefetchService::RegisterProfilePrefs(registry);
  blocked_content::SafeBrowsingTriggeredPopupBlocker::RegisterProfilePrefs(
      registry);
  security_interstitials::InsecureFormBlockingPage::RegisterProfilePrefs(
      registry);
  segmentation_platform::SegmentationPlatformService::RegisterProfilePrefs(
      registry);
  segmentation_platform::DeviceSwitcherResultDispatcher::RegisterProfilePrefs(
      registry);
  SessionStartupPref::RegisterProfilePrefs(registry);
  SharingSyncPreference::RegisterProfilePrefs(registry);
  site_engagement::SiteEngagementService::RegisterProfilePrefs(registry);
  storage::RegisterProfilePrefs(registry);
  sync_sessions::SessionSyncPrefs::RegisterProfilePrefs(registry);
  syncer::DeviceInfoPrefs::RegisterProfilePrefs(registry);
  syncer::SyncPrefs::RegisterProfilePrefs(registry);
  syncer::SyncTransportDataPrefs::RegisterProfilePrefs(registry);
  TemplateURLPrepopulateData::RegisterProfilePrefs(registry);
  tpcd::experiment::RegisterProfilePrefs(registry);
  translate::TranslatePrefs::RegisterProfilePrefs(registry);
  omnibox::RegisterProfilePrefs(registry);
  ZeroSuggestProvider::RegisterProfilePrefs(registry);

#if !BUILDFLAG(IS_ANDROID) && BUILDFLAG(GOOGLE_CHROME_BRANDING)
  promos_utils::RegisterProfilePrefs(registry);
#endif  // !BUILDFLAG(IS_ANDROID) && BUILDFLAG(GOOGLE_CHROME_BRANDING)

#if BUILDFLAG(ENABLE_SESSION_SERVICE)
  RegisterSessionServiceLogProfilePrefs(registry);
  SessionDataService::RegisterProfilePrefs(registry);
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
  ExtensionWebUI::RegisterProfilePrefs(registry);
  RegisterAnimationPolicyPrefs(registry);
  extensions::ActivityLog::RegisterProfilePrefs(registry);
  extensions::AudioAPI::RegisterUserPrefs(registry);
  extensions::ExtensionPrefs::RegisterProfilePrefs(registry);
  extensions::ExtensionsUI::RegisterProfilePrefs(registry);
#if BUILDFLAG(IS_CHROMEOS_ASH)
  extensions::shared_storage::RegisterProfilePrefs(registry);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)
  extensions::PermissionsManager::RegisterProfilePrefs(registry);
  extensions::RuntimeAPI::RegisterPrefs(registry);
  // TODO(devlin): This would be more inline with the other calls here if it
  // were nested in either a class or separate namespace with a simple
  // Register[Profile]Prefs() name.
  extensions::RegisterSettingsOverriddenUiPrefs(registry);
  update_client::RegisterProfilePrefs(registry);
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

#if BUILDFLAG(ENABLE_PDF)
  registry->RegisterListPref(prefs::kPdfLocalFileAccessAllowedForDomains,
                             base::Value::List());
  registry->RegisterBooleanPref(prefs::kPdfUseSkiaRendererEnabled, true);
#endif  // BUILDFLAG(ENABLE_PDF)

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
  printing::PolicySettings::RegisterProfilePrefs(registry);
  printing::PrintPreviewStickySettings::RegisterProfilePrefs(registry);
#endif

#if BUILDFLAG(ENABLE_RLZ)
  ChromeRLZTrackerDelegate::RegisterProfilePrefs(registry);
#endif

#if BUILDFLAG(ENABLE_SUPERVISED_USERS)
  supervised_user::RegisterProfilePrefs(registry);
#endif

#if BUILDFLAG(ENABLE_FEED_V2)
  feed::prefs::RegisterFeedSharedProfilePrefs(registry);
  feed::RegisterProfilePrefs(registry);
#endif

#if BUILDFLAG(IS_ANDROID)
  cdm::MediaDrmStorageImpl::RegisterProfilePrefs(registry);
  KnownInterceptionDisclosureInfoBarDelegate::RegisterProfilePrefs(registry);
  MediaDrmOriginIdManager::RegisterProfilePrefs(registry);
  NotificationChannelsProviderAndroid::RegisterProfilePrefs(registry);
  ntp_tiles::PopularSitesImpl::RegisterProfilePrefs(registry);
  OomInterventionDecider::RegisterProfilePrefs(registry);
  PartnerBookmarksShim::RegisterProfilePrefs(registry);
  permissions::GeolocationPermissionContextAndroid::RegisterProfilePrefs(
      registry);
  query_tiles::RegisterPrefs(registry);
  readaloud::RegisterProfilePrefs(registry);
  RecentTabsPagePrefs::RegisterProfilePrefs(registry);
  usage_stats::UsageStatsBridge::RegisterProfilePrefs(registry);
  variations::VariationsService::RegisterProfilePrefs(registry);
  webapps::InstallPromptPrefs::RegisterProfilePrefs(registry);
#else  // BUILDFLAG(IS_ANDROID)
  bookmarks_webui::RegisterProfilePrefs(registry);
  browser_sync::ForeignSessionHandler::RegisterProfilePrefs(registry);
  BrowserFeaturePromoStorageService::RegisterProfilePrefs(registry);
  captions::LiveTranslateController::RegisterProfilePrefs(registry);
  CartService::RegisterProfilePrefs(registry);
  ChromeAuthenticatorRequestDelegate::RegisterProfilePrefs(registry);
  commerce::CommerceUiTabHelper::RegisterProfilePrefs(registry);
  companion::PromoHandler::RegisterProfilePrefs(registry);
  DeviceServiceImpl::RegisterProfilePrefs(registry);
  DevToolsWindow::RegisterProfilePrefs(registry);
  DriveService::RegisterProfilePrefs(registry);
  enterprise_connectors::RegisterProfilePrefs(registry);
  extensions::CommandService::RegisterProfilePrefs(registry);
  extensions::TabsCaptureVisibleTabFunction::RegisterProfilePrefs(registry);
  first_run::RegisterProfilePrefs(registry);
  gcm::RegisterProfilePrefs(registry);
  HatsServiceDesktop::RegisterProfilePrefs(registry);
  NtpCustomBackgroundService::RegisterProfilePrefs(registry);
  media_router::RegisterAccessCodeProfilePrefs(registry);
  media_router::RegisterProfilePrefs(registry);
  NewTabPageHandler::RegisterProfilePrefs(registry);
  NewTabPageUI::RegisterProfilePrefs(registry);
  ntp::SafeBrowsingHandler::RegisterProfilePrefs(registry);
  ntp_tiles::CustomLinksManagerImpl::RegisterProfilePrefs(registry);
  PhotosService::RegisterProfilePrefs(registry);
  PinnedTabCodec::RegisterProfilePrefs(registry);
  policy::DeveloperToolsPolicyHandler::RegisterProfilePrefs(registry);
  PromoService::RegisterProfilePrefs(registry);
  RecipesService::RegisterProfilePrefs(registry);
  RegisterReadAnythingProfilePrefs(registry);
  RegisterSafetyHubProfilePrefs(registry);
  settings::SettingsUI::RegisterProfilePrefs(registry);
  send_tab_to_self::RegisterProfilePrefs(registry);
  signin::RegisterProfilePrefs(registry);
  StartupBrowserCreator::RegisterProfilePrefs(registry);
  TabResumptionPageHandler::RegisterProfilePrefs(registry);
  tab_organization_prefs::RegisterProfilePrefs(registry);
  tab_search_prefs::RegisterProfilePrefs(registry);
  ThemeColorPickerHandler::RegisterProfilePrefs(registry);
  toolbar::RegisterProfilePrefs(registry);
  UnifiedAutoplayConfig::RegisterProfilePrefs(registry);
  user_notes::RegisterProfilePrefs(registry);

#if !BUILDFLAG(IS_CHROMEOS_LACROS)
  captions::LiveCaptionController::RegisterProfilePrefs(registry);
#endif  // !BUILDFLAG(IS_CHROMEOS_LACROS)
#endif  // BUILDFLAG(IS_ANDROID)

#if BUILDFLAG(IS_CHROMEOS)
  extensions::DocumentScanAPIHandler::RegisterProfilePrefs(registry);
  extensions::login_api::RegisterProfilePrefs(registry);
  extensions::platform_keys::RegisterProfilePrefs(registry);
  certificate_manager::CertificatesHandler::RegisterProfilePrefs(registry);
  chromeos::cloud_storage::RegisterProfilePrefs(registry);
  chromeos::cloud_upload::RegisterProfilePrefs(registry);
  policy::PolicyCertService::RegisterProfilePrefs(registry);
  quickoffice::RegisterProfilePrefs(registry);
  registry->RegisterBooleanPref(prefs::kDeskAPIThirdPartyAccessEnabled, false);
  registry->RegisterBooleanPref(prefs::kDeskAPIDeskSaveAndShareEnabled, false);
  registry->RegisterListPref(prefs::kDeskAPIThirdPartyAllowlist);
  registry->RegisterBooleanPref(prefs::kInsightsExtensionEnabled, false);
  registry->RegisterBooleanPref(prefs::kEssentialSearchEnabled, false);
  registry->RegisterBooleanPref(prefs::kLastEssentialSearchValue, false);
  // By default showing Sync Consent is set to true. It can changed by policy.
  registry->RegisterBooleanPref(prefs::kEnableSyncConsent, true);
  registry->RegisterListPref(
      chromeos::prefs::kKeepFullscreenWithoutNotificationUrlAllowList,
      PrefRegistry::PUBLIC);
  registry->RegisterBooleanPref(policy::policy_prefs::kFloatingWorkspaceEnabled,
                                false);
  ::reporting::RegisterProfilePrefs(registry);
#if BUILDFLAG(USE_CUPS)
  extensions::PrintingAPIHandler::RegisterProfilePrefs(registry);
#endif  // BUILDFLAG(USE_CUPS)
#endif  // BUILDFLAG(IS_CHROMEOS)

#if BUILDFLAG(IS_CHROMEOS_ASH)
  app_list::AppListSyncableService::RegisterProfilePrefs(registry);
  apps::AlmanacFetcher::RegisterProfilePrefs(registry);
  apps::AppPlatformMetricsService::RegisterProfilePrefs(registry);
  apps::AppPreloadService::RegisterProfilePrefs(registry);
  apps::deduplication::AppDeduplicationService::RegisterProfilePrefs(registry);
  apps::webapk_prefs::RegisterProfilePrefs(registry);
  arc::prefs::RegisterProfilePrefs(registry);
  ArcAppListPrefs::RegisterProfilePrefs(registry);
  arc::ArcBootPhaseMonitorBridge::RegisterProfilePrefs(registry);
  ash::AccountAppsAvailability::RegisterPrefs(registry);
  account_manager::AccountManager::RegisterPrefs(registry);
  ash::ApkWebAppService::RegisterProfilePrefs(registry);
  ash::app_time::AppActivityRegistry::RegisterProfilePrefs(registry);
  ash::app_time::AppTimeController::RegisterProfilePrefs(registry);
  ash::AshProxyMonitor::RegisterProfilePrefs(registry);
  ash::assistant::prefs::RegisterProfilePrefs(registry);
  ash::auth::AuthFactorConfig::RegisterPrefs(registry);
  ash::bluetooth::DebugLogsManager::RegisterPrefs(registry);
  ash::bluetooth_config::BluetoothPowerControllerImpl::RegisterProfilePrefs(
      registry);
  ash::HatsBluetoothRevampTriggerImpl::RegisterProfilePrefs(registry);
  ash::ChromeUserManagerImpl::RegisterProfilePrefs(registry);
  ash::ClientAppMetadataProviderService::RegisterProfilePrefs(registry);
  ash::CupsPrintersManager::RegisterProfilePrefs(registry);
  ash::device_sync::RegisterProfilePrefs(registry);
  ash::FamilyUserChromeActivityMetrics::RegisterProfilePrefs(registry);
  ash::FamilyUserMetricsService::RegisterProfilePrefs(registry);
  ash::FamilyUserSessionMetrics::RegisterProfilePrefs(registry);
  ash::InlineLoginHandlerImpl::RegisterProfilePrefs(registry);
  ash::first_run::RegisterProfilePrefs(registry);
  ash::file_system_provider::RegisterProfilePrefs(registry);
  ash::full_restore::RegisterProfilePrefs(registry);
  ash::KerberosCredentialsManager::RegisterProfilePrefs(registry);
  ash::multidevice_setup::MultiDeviceSetupService::RegisterProfilePrefs(
      registry);
  ash::NetworkMetadataStore::RegisterPrefs(registry);
  ash::ReleaseNotesStorage::RegisterProfilePrefs(registry);
  ash::HelpAppNotificationController::RegisterProfilePrefs(registry);
  ash::quick_unlock::FingerprintStorage::RegisterProfilePrefs(registry);
  ash::quick_unlock::PinStoragePrefs::RegisterProfilePrefs(registry);
  ash::Preferences::RegisterProfilePrefs(registry);
  ash::EnterprisePrintersProvider::RegisterProfilePrefs(registry);
  ash::parent_access::ParentAccessService::RegisterProfilePrefs(registry);
  quick_answers::prefs::RegisterProfilePrefs(registry);
  ash::quick_unlock::RegisterProfilePrefs(registry);
  ash::RegisterSamlProfilePrefs(registry);
  ash::ScreenTimeController::RegisterProfilePrefs(registry);
  ash::EduCoexistenceConsentInvalidationController::RegisterProfilePrefs(
      registry);
  ash::EduCoexistenceLoginHandler::RegisterProfilePrefs(registry);
  ash::SigninErrorNotifier::RegisterPrefs(registry);
  ash::ServicesCustomizationDocument::RegisterProfilePrefs(registry);
  ash::settings::OSSettingsUI::RegisterProfilePrefs(registry);
  ash::StartupUtils::RegisterOobeProfilePrefs(registry);
  ash::user_image::prefs::RegisterProfilePrefs(registry);
  ash::UserImageSyncObserver::RegisterProfilePrefs(registry);
  ChromeMetricsServiceClient::RegisterProfilePrefs(registry);
  crostini::prefs::RegisterProfilePrefs(registry);
  flags_ui::PrefServiceFlagsStorage::RegisterProfilePrefs(registry);
  guest_os::prefs::RegisterProfilePrefs(registry);
  lock_screen_apps::StateController::RegisterProfilePrefs(registry);
  plugin_vm::prefs::RegisterProfilePrefs(registry);
  policy::ArcAppInstallEventLogger::RegisterProfilePrefs(registry);
  policy::AppInstallEventLogManagerWrapper::RegisterProfilePrefs(registry);
  policy::StatusCollector::RegisterProfilePrefs(registry);
  ash::SystemProxyManager::RegisterProfilePrefs(registry);
  ChromeShelfPrefs::RegisterProfilePrefs(registry);
  ::onc::RegisterProfilePrefs(registry);
  ash::cert_provisioning::RegisterProfilePrefs(registry);
  borealis::prefs::RegisterProfilePrefs(registry);
  ash::ChromeScanningAppDelegate::RegisterProfilePrefs(registry);
  ProjectorAppClientImpl::RegisterProfilePrefs(registry);
  ash::floating_workspace_util::RegisterProfilePrefs(registry);
  policy::RebootNotificationsScheduler::RegisterProfilePrefs(registry);
  ash::KioskChromeAppManager::RegisterProfilePrefs(registry);
  file_manager::file_tasks::RegisterProfilePrefs(registry);
  file_manager::prefs::RegisterProfilePrefs(registry);
  bruschetta::prefs::RegisterProfilePrefs(registry);
  wallpaper_handlers::prefs::RegisterProfilePrefs(registry);
  ash::reporting::RegisterProfilePrefs(registry);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

#if BUILDFLAG(IS_CHROMEOS_LACROS)
  lacros_prefs::RegisterProfilePrefs(registry);
  chromeos::ProxyConfigServiceLacros::RegisterProfilePrefs(registry);
  lacros_prefs::RegisterExtensionControlledAshPrefs(registry);
  KioskSessionServiceLacros::RegisterProfilePrefs(registry);
  apps::WebsiteMetricsServiceLacros::RegisterProfilePrefs(registry);
#endif

#if BUILDFLAG(IS_WIN)
  CdmPrefServiceHelper::RegisterProfilePrefs(registry);
  FontPrewarmerTabHelper::RegisterProfilePrefs(registry);
  NetworkProfileBubble::RegisterProfilePrefs(registry);
#endif

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX) || \
    BUILDFLAG(IS_CHROMEOS_ASH)
  device_signals::RegisterProfilePrefs(registry);
#endif  // BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX) ||
        // BUILDFLAG(IS_CHROMEOS_ASH)

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
  browser_switcher::BrowserSwitcherPrefs::RegisterProfilePrefs(registry);
  enterprise_signin::RegisterProfilePrefs(registry);
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS) && !BUILDFLAG(IS_CHROMEOS_ASH)
  preinstalled_apps::RegisterProfilePrefs(registry);
#endif

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_CHROMEOS)
  sharing_hub::RegisterProfilePrefs(registry);
#endif

#if defined(TOOLKIT_VIEWS)
  accessibility_prefs::RegisterInvertBubbleUserPrefs(registry);
  side_search_prefs::RegisterProfilePrefs(registry);
  RegisterBrowserViewProfilePrefs(registry);
#endif

#if BUILDFLAG(ENABLE_LENS_DESKTOP)
  registry->RegisterBooleanPref(
      prefs::kLensRegionSearchEnabled, true,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterBooleanPref(prefs::kLensDesktopNTPSearchEnabled, true);
#endif

#if !BUILDFLAG(IS_ANDROID)
  registry->RegisterBooleanPref(
      webauthn::pref_names::kRemoteProxiedRequestsAllowed, false);

  registry->RegisterStringPref(
      webauthn::pref_names::kLastUsedPairingFromSyncPublicKey, "");

  side_panel_prefs::RegisterProfilePrefs(registry);
#endif

  registry->RegisterBooleanPref(webauthn::pref_names::kAllowWithBrokenCerts,
                                false);

  registry->RegisterBooleanPref(prefs::kPrivacyGuideViewed, false);

  RegisterProfilePrefsForMigration(registry);

#if !BUILDFLAG(IS_ANDROID)
  registry->RegisterIntegerPref(prefs::kMemorySaverChipExpandedCount, 0);
  registry->RegisterTimePref(prefs::kLastMemorySaverChipExpandedTimestamp,
                             base::Time());
#endif

#if BUILDFLAG(IS_ANDROID)
  registry->RegisterBooleanPref(prefs::kVirtualKeyboardResizesLayoutByDefault,
                                false);
#endif

  registry->RegisterTimePref(prefs::kDIPSTimerLastUpdate, base::Time());

#if BUILDFLAG(ENABLE_SCREEN_AI_SERVICE)
  registry->RegisterBooleanPref(prefs::kAccessibilityPdfOcrAlwaysActive, true);
#endif  // BUILDFLAG(ENABLE_SCREEN_AI_SERVICE)

#if BUILDFLAG(IS_CHROMEOS_ASH)
  registry->RegisterBooleanPref(kClearUserDataDir1Pref, false);
#endif

  registry->RegisterBooleanPref(prefs::kBlockTruncatedCookies, true);
  registry->RegisterBooleanPref(
      prefs::kManagedPrivateNetworkAccessRestrictionsEnabled, false);

#if BUILDFLAG(ENTERPRISE_DATA_CONTROLS)
  data_controls::RegisterProfilePrefs(registry);
#endif  // BUILDFLAG(ENTERPRISE_DATA_CONTROLS)

#if BUILDFLAG(IS_WIN)
  registry->RegisterBooleanPref(prefs::kNativeHostsExecutablesLaunchDirectly,
                                false);
#endif  // BUILDFLAG(IS_WIN)

#if BUILDFLAG(ENABLE_COMPOSE)
  registry->RegisterBooleanPref(prefs::kPrefHasCompletedComposeFRE, false);
#endif
}

void RegisterUserProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  RegisterUserProfilePrefs(registry, g_browser_process->GetApplicationLocale());
}

void RegisterUserProfilePrefs(user_prefs::PrefRegistrySyncable* registry,
                              const std::string& locale) {
  RegisterProfilePrefs(registry, locale);

#if BUILDFLAG(IS_ANDROID)
  ::android::RegisterUserProfilePrefs(registry);
#endif
#if BUILDFLAG(IS_CHROMEOS_ASH)
  ash::RegisterUserProfilePrefs(registry, locale);
  ash::TokenHandleFetcher::RegisterPrefs(registry);
#endif
}

void RegisterScreenshotPrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kDisableScreenshots, false);
}

#if BUILDFLAG(IS_CHROMEOS_ASH)
void RegisterSigninProfilePrefs(user_prefs::PrefRegistrySyncable* registry,
                                std::string_view country) {
  RegisterProfilePrefs(registry, g_browser_process->GetApplicationLocale());
  ash::RegisterSigninProfilePrefs(registry, country);
}

#endif

// This method should be periodically pruned of year+ old migrations.
// See chrome/browser/prefs/README.md for details.
void MigrateObsoleteLocalStatePrefs(PrefService* local_state) {
  // IMPORTANT NOTE: This code is *not* run on iOS Chrome. If a pref is migrated
  // or cleared here, and that pref is also used in iOS Chrome, it may also need
  // to be migrated or cleared specifically for iOS as well. This could be by
  // doing the migration in feature code that's called by all platforms instead
  // of here, or by calling migration code in the appropriate place for iOS
  // specifically, e.g. ios/chrome/browser/shared/model/prefs/browser_prefs.mm.

  // BEGIN_MIGRATE_OBSOLETE_LOCAL_STATE_PREFS
  // Please don't delete the preceding line. It is used by PRESUBMIT.py.

// Added 04/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  local_state->ClearPref(kEasyUnlockHardlockState);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

  // Added 04/2023.
  local_state->ClearPref(kTypeSubscribedForInvalidations);
  local_state->ClearPref(kActiveRegistrationToken);
  local_state->ClearPref(kFCMInvalidationClientIDCache);

// Added 04/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  local_state->ClearPref(kConsolidatedConsentTrial);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Added 04/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  local_state->ClearPref(kEnrollmentIdUploadedOnChromad);
  local_state->ClearPref(kLastChromadMigrationAttemptTime);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

  // Added 05/2023.
  local_state->ClearPref(kForceEnablePepperVideoDecoderDevAPI);

  // Added 05/2023.
  local_state->ClearPref(kUseMojoVideoDecoderForPepperAllowed);

  // Added 05/2023.
  local_state->ClearPref(kPPAPISharedImagesSwapChainAllowed);

// Added 05/2023.
#if BUILDFLAG(ENABLE_SCREEN_AI_SERVICE)
  local_state->ClearPref(kScreenAIScheduledDeletionTimePrefName);
#endif

// Added 05/2023.
#if BUILDFLAG(IS_ANDROID)
  local_state->ClearPref(kWeeklyStatsReportingTimestamp);
#endif

// Added 05/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  local_state->ClearPref(kSupervisedUsersFirstRun);
  local_state->ClearPref(kSupervisedUserSyncId);
  local_state->ClearPref(kSupervisedUserManagers);
  local_state->ClearPref(kSupervisedUserManagerNames);
  local_state->ClearPref(kSupervisedUserManagerDisplayEmails);

  local_state->ClearPref(kSupervisedUserPasswordSchema);
  local_state->ClearPref(kSupervisedUserPasswordSalt);
  local_state->ClearPref(kSupervisedUserPasswordRevision);

  local_state->ClearPref(kSupervisedUserNeedPasswordUpdate);
  local_state->ClearPref(kSupervisedUserIncompleteKey);
#endif

// Added 07/2023.
#if !BUILDFLAG(IS_ANDROID)
  local_state->ClearPref(kLegacyHoverCardImagesEnabled);
#endif

  // Added 07/2023.
  local_state->ClearPref(kShutdownNumProcesses);
  local_state->ClearPref(kShutdownNumProcessesSlow);
  local_state->ClearPref(kShutdownType);

  // Added 09/2023.
#if BUILDFLAG(IS_WIN)
  local_state->ClearPref(kSwReporter);
  local_state->ClearPref(kChromeCleaner);
#endif

// Added 09/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  local_state->ClearPref(kGestureEducationNotificationShown);
#endif

// Added 11/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  local_state->ClearPref(kIsolatedWebAppsEnabled);
#endif

  // Added 12/2023.
  local_state->ClearPref(kPrivacyBudgetReportedReidBlocks);

  // Added 01/2024.
  local_state->ClearPref(kPPAPISharedImagesForVideoDecoderAllowed);

// Added 01/2024.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  local_state->ClearPref(kExtendedFkeysModifier);
#endif

// Added 02/2024
#if BUILDFLAG(IS_MAC)
  local_state->ClearPref(kScreenTimeEnabled);
#endif

  // Added 02/2024
  local_state->ClearPref(kSearchEnginesChoiceProfile);

#if BUILDFLAG(IS_WIN)
  // Deprecated 02/2024.
  local_state->ClearPref(kOsCryptAppBoundFixedDataPrefName);
#endif

#if BUILDFLAG(IS_CHROMEOS_ASH)
  // Added 03/2024.
  local_state->ClearPref(kOobeGuestAcceptedTos);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

  // Please don't delete the following line. It is used by PRESUBMIT.py.
  // END_MIGRATE_OBSOLETE_LOCAL_STATE_PREFS

  // IMPORTANT NOTE: This code is *not* run on iOS Chrome. If a pref is migrated
  // or cleared here, and that pref is also used in iOS Chrome, it may also need
  // to be migrated or cleared specifically for iOS as well. This could be by
  // doing the migration in feature code that's called by all platforms instead
  // of here, or by calling migration code in the appropriate place for iOS
  // specifically, e.g. ios/chrome/browser/shared/model/prefs/browser_prefs.mm.
}

// This method should be periodically pruned of year+ old migrations.
// See chrome/browser/prefs/README.md for details.
void MigrateObsoleteProfilePrefs(PrefService* profile_prefs,
                                 const base::FilePath& profile_path) {
  // IMPORTANT NOTE: This code is *not* run on iOS Chrome. If a pref is migrated
  // or cleared here, and that pref is also used in iOS Chrome, it may also need
  // to be migrated or cleared specifically for iOS as well. This could be by
  // doing the migration in feature code that's called by all platforms instead
  // of here, or by calling migration code in the appropriate place for iOS
  // specifically, e.g. ios/chrome/browser/shared/model/prefs/browser_prefs.mm.

  // BEGIN_MIGRATE_OBSOLETE_PROFILE_PREFS
  // Please don't delete the preceding line. It is used by PRESUBMIT.py.

  // Check MigrateDeprecatedAutofillPrefs() to see if this is safe to remove.
  autofill::prefs::MigrateDeprecatedAutofillPrefs(profile_prefs);

  // Added 3/2020.
  // TODO(crbug.com/1062698): Remove this once the privacy settings redesign
  // is fully launched.
  chrome_browser_net::secure_dns::MigrateProbesSettingToOrFromBackup(
      profile_prefs);

  // Once this migration is complete, the tracked preference
  // `kGoogleServicesLastSyncingAccountIdDeprecated` can be removed.
  if (profile_prefs->HasPrefPath(
          prefs::kGoogleServicesLastSyncingAccountIdDeprecated)) {
    std::string account_id = profile_prefs->GetString(
        prefs::kGoogleServicesLastSyncingAccountIdDeprecated);
    profile_prefs->ClearPref(
        prefs::kGoogleServicesLastSyncingAccountIdDeprecated);
    bool is_email = account_id.find('@') != std::string::npos;
    if (!is_email && !account_id.empty()) {
      profile_prefs->SetString(prefs::kGoogleServicesLastSyncingGaiaId,
                               account_id);
    }
  }

  // TODO(326079444): After experiment is over, update the deprecated date and
  // allow this to be cleaned up.
#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_CHROMEOS_ASH)
  MigrateDefaultBrowserLastDeclinedPref(profile_prefs);
#endif

// Added 04/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  profile_prefs->ClearPref(kBentoBarEnabled);
  profile_prefs->ClearPref(kUserHasUsedDesksRecently);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Added 04/2023.
#if BUILDFLAG(IS_ANDROID)
  profile_prefs->ClearPref(kBackoff);
  profile_prefs->ClearPref(kUserSettingEnabled);
  profile_prefs->ClearPref(kLimitlessPrefetchingEnabledTimePref);
  profile_prefs->ClearPref(kPrefetchTestingHeaderPref);
  profile_prefs->ClearPref(kEnabledByServer);
  profile_prefs->ClearPref(kNextForbiddenCheckTimePref);
  profile_prefs->ClearPref(kPrefetchCachedGCMToken);
#endif

  // Added 04/2023.
  profile_prefs->ClearPref(kTypeSubscribedForInvalidations);
  profile_prefs->ClearPref(kActiveRegistrationToken);
  profile_prefs->ClearPref(kFCMInvalidationClientIDCache);

  // Added 04/2023.
#if BUILDFLAG(IS_ANDROID)
  ntp_snippets::prefs::MigrateObsoleteProfilePrefsApril2023(profile_prefs);
#endif

// Added 04/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  profile_prefs->ClearPref(kOfficeFilesAlwaysMove);
  profile_prefs->ClearPref(kOfficeMoveConfirmationShown);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Added 04/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  profile_prefs->ClearPref(kProximityAuthIsChromeOSLoginEnabled);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Added 05/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  profile_prefs->ClearPref(kSmartLockSigninAllowed);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Added 05/2023.
#if BUILDFLAG(IS_ANDROID)
  profile_prefs->ClearPref(kVideoTutorialsPreferredLocaleKey);
  profile_prefs->ClearPref(kVideoTutorialsLastUpdatedTimeKey);
#endif  // BUILDFLAG(IS_ANDROID

// Added 05/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  profile_prefs->ClearPref(kOfficeSetupComplete);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Added 05/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  profile_prefs->ClearPref(kEventRemappedToRightClick);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Added 05/2023.
#if BUILDFLAG(IS_ANDROID)
  profile_prefs->ClearPref(kTimesUPMAuthErrorShown);
#endif  // BUILDFLAG(IS_ANDROID)

// Added 05/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  profile_prefs->ClearPref(kSamlPasswordSyncToken);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Added 06/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  profile_prefs->ClearPref(kWebAppCalculatorAppErasureFixAppliedPref);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

  // Added 06/2023.
  profile_prefs->ClearPref(kWebAppsExtensionIDs);

  // Added 06/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  profile_prefs->ClearPref(kOsSyncPrefsMigrated);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

  // Added 06/2023.
#if !BUILDFLAG(IS_ANDROID)
  profile_prefs->ClearPref(kShouldShowSidePanelBookmarkTab);
#endif  // !BUILDFLAG(IS_ANDROID)

  // Added 06/2023.
#if BUILDFLAG(ENABLE_FEED_V2)
  profile_prefs->ClearPref(kVideoPreviewsType);
#endif  // BUILDFLAG(ENABLE_FEED_V2)

  // Added 06/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  profile_prefs->ClearPref(kGaiaLastOnlineSignInTime);
  profile_prefs->ClearPref(kSAMLLastGAIASignInTime);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

  // Added 06/2023.
#if BUILDFLAG(IS_ANDROID)
  profile_prefs->ClearPref(kPrefExplicitLanguageAskShown);
#endif  // BUILDFLAG(IS_ANDROID)

// Added 07/2023.
#if !BUILDFLAG(IS_CHROMEOS_ASH)
  profile_prefs->ClearPref(kUnifiedConsentMigrationState);
#endif

  // Added 07/2023.
  profile_prefs->ClearPref(kPasswordsGroupingInfoRequested);

  // Added 07/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  profile_prefs->ClearPref(kPowerMetricsIdleScreenOffCount);
  profile_prefs->ClearPref(kPowerMetricsIdleSuspendCount);
  profile_prefs->ClearPref(kPowerMetricsLidClosedSuspendCount);
#endif
  syncer::SyncPrefs::MigrateAutofillWalletImportEnabledPref(profile_prefs);

// Added 07/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  profile_prefs->ClearPref(kHatsPrivacyHubBaselineIsSelected);
  profile_prefs->ClearPref(kHatsPrivacyHubBaselineCycleEndTs);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

// Added 07/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  profile_prefs->ClearPref(kClearUserDataDir1Pref);
#endif

  // Added 08/2023.
  invalidation::InvalidatorRegistrarWithMemory::ClearDeprecatedPrefs(
      profile_prefs);
  invalidation::PerUserTopicSubscriptionManager::ClearDeprecatedPrefs(
      profile_prefs);
  invalidation::FCMInvalidationService::ClearDeprecatedPrefs(profile_prefs);

  // Added 08/2023.
  profile_prefs->ClearPref(kDriveFsBulkPinningMaxQueueSize);

  // Added 09/2023.
  profile_prefs->ClearPref(kPrivacySandboxM1Unrestricted);
#if BUILDFLAG(IS_WIN)
  profile_prefs->ClearPref(kSwReporter);
  profile_prefs->ClearPref(kSettingsResetPrompt);
  profile_prefs->ClearPref(kChromeCleaner);
#endif
  profile_prefs->ClearPref(kDownloadBubbleEnabled);
  profile_prefs->ClearPref(kPrivacySandboxManuallyControlled);
#if BUILDFLAG(IS_CHROMEOS_ASH)
  profile_prefs->ClearPref(kSyncInitialSyncFeatureSetupCompleteOnAsh);
#endif
#if BUILDFLAG(IS_ANDROID)
  profile_prefs->ClearPref(kSettingsMigratedToUPM);
#endif

  // Added 10/2023.
  ClearSyncRequestedPrefAndMaybeMigrate(profile_prefs);

// Added 10/2023.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  profile_prefs->ClearPref(kLastSuccessfulDomainPref);
  profile_prefs->ClearPref(kShouldAttemptReenable);
  profile_prefs->ClearPref(kAudioVolumePercent);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

  // Added 10/2023.
#if BUILDFLAG(IS_CHROMEOS)
  profile_prefs->ClearPref(kSupportedLinksAppPrefsKey);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

#if BUILDFLAG(IS_CHROMEOS_ASH)
  // Added 10/2023.
  profile_prefs->ClearPref(kNightLightCachedLatitude);
  profile_prefs->ClearPref(kNightLightCachedLongitude);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

  // Added 11/2023.
  profile_prefs->ClearPref(kPrivacySandboxAntiAbuseInitialized);

  // Added 11/2023.
  profile_prefs->ClearPref(kWebRTCAllowLegacyTLSProtocols);

#if BUILDFLAG(IS_CHROMEOS_ASH)
  // Added 11/2023.
  profile_prefs->ClearPref(kSystemTrayExpanded);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

#if BUILDFLAG(IS_CHROMEOS_ASH)
  // Added 11/2023.
  profile_prefs->ClearPref(kUserGeolocationAllowed);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

#if !BUILDFLAG(IS_ANDROID)
  // Added 11/2023.
  password_manager::features_util::MigrateOptInPrefToSyncSelectedTypes(
      profile_prefs);
#endif  // !BUILDFLAG(IS_ANDROID)

#if BUILDFLAG(IS_ANDROID)
  // Added 11/2023, but DO NOT REMOVE after the usual year!
  // TODO(crbug.com/1445497): The pref kPasswordsUseUPMLocalAndSeparateStores
  // and this call (to compute said pref) should be removed once
  // kUnifiedPasswordManagerLocalPasswordsAndroidWithMigration is launched and
  // enough clients have migrated. UsesSplitStoresAndUPMForLocal() should be
  // updated to check the GmsCoreVersion directly instead of the pref, or might
  // be removed entirely, depending how the outdated GmsCore case is handled.
  password_manager_android_util::SetUsesSplitStoresAndUPMForLocal(profile_prefs,
                                                                  profile_path);
#endif

  // Added 11/2023.
  profile_prefs->ClearPref(kPasswordChangeSuccessTrackerFlows);
  profile_prefs->ClearPref(kPasswordChangeSuccessTrackerVersion);

#if BUILDFLAG(IS_CHROMEOS_ASH)
  // Added 11/2023.
  profile_prefs->ClearPref(kImageSearchPrivacyNotice);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

  // Added 11/2023.
  profile_prefs->ClearPref(kWebAndAppActivityEnabledForShopping);

#if !BUILDFLAG(IS_ANDROID)
  // Added 12/2023.
  password_manager::features_util::MigrateDeclinedSaveOptInToExplicitOptOut(
      profile_prefs);
#endif  // !BUILDFLAG(IS_ANDROID)

#if BUILDFLAG(IS_ANDROID)
  // Added 12/2023.
  profile_prefs->ClearPref(kTemplatesRandomOrder);
#endif

#if BUILDFLAG(IS_ANDROID)
  // Added 12/2023.
  profile_prefs->ClearPref(kDesktopSitePeripheralSettingEnabled);
  profile_prefs->ClearPref(kDesktopSiteDisplaySettingEnabled);
#endif

  // Added 12/2023.
  profile_prefs->ClearPref(kDownloadDuplicateFilePromptEnabled);

  // Added 12/2023.
  profile_prefs->ClearPref(kModelQualityLoggingClientId);

  // Added 12/2023.
  // Moving the `kExplicitBrowserSignin` from sync/ to signin/.
  // If the sync (old) pref still exists, copy it to signin (new),
  // and clear the sync part of the pref.
  if (profile_prefs->HasPrefPath(kSync_ExplicitBrowserSignin)) {
    profile_prefs->SetBoolean(
        prefs::kExplicitBrowserSignin,
        profile_prefs->GetBoolean(kSync_ExplicitBrowserSignin));
    profile_prefs->ClearPref(kSync_ExplicitBrowserSignin);
  }

  // Added 01/2024.
  profile_prefs->ClearPref(kPrivacySandboxPageViewed);

  // Added 01/2024.
  profile_prefs->ClearPref(kPrivacySandboxApisEnabledV2);
  profile_prefs->ClearPref(kPrivacySandboxManuallyControlledV2);

  // Added 01/2024.
#if BUILDFLAG(ENABLE_COMPOSE)
  profile_prefs->ClearPref(kPrefHasAcceptedComposeConsent);
  profile_prefs->ClearPref(kAutofillAssistanceEnabled);
#endif

  // Added 01/2024.
  profile_prefs->ClearPref(kSyncedLastTimePasswordCheckCompleted);

  // Added 01/2024.
  profile_prefs->ClearPref(kDownloadBubbleIphSuppression);

  // Added 01/2024.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  profile_prefs->ClearPref(kPersistedSystemExtensions);
  profile_prefs->ClearPref(kBorealisVmTokenHash);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

#if !BUILDFLAG(IS_ANDROID)
  // Added 1/2024.
  performance_manager::user_tuning::prefs::MigrateTabDiscardingExceptionsPref(
      profile_prefs);
#endif

  // Added 01/2024.
  profile_prefs->ClearPref(kNtpShownPage);
  profile_prefs->ClearPref(kNtpAppPageNames);

  // Added 01/2024.
#if BUILDFLAG(IS_WIN)
  profile_prefs->ClearPref(kSearchResultsPagePrimaryFontsPref);
  profile_prefs->ClearPref(kSearchResultsPageFallbackFontsPref);
#endif

  // Added 01/2024.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  profile_prefs->ClearPref(kUpdateNotificationLastShownMilestone);
#endif

  // Added 01/2024.
#if BUILDFLAG(IS_ANDROID)
  profile_prefs->ClearPref(kSavePasswordsSuspendedByError);
#endif

  // Added 02/2024
  profile_prefs->ClearPref(kSafeBrowsingDeepScanPromptSeen);
  profile_prefs->ClearPref(kSafeBrowsingEsbEnabledTimestamp);

  // Added 02/2024.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  for (const char* pref : kWelcomeTourTimeBucketsOfFirstInteractions) {
    profile_prefs->ClearPref(pref);
  }

  // Added 02/2024.
  profile_prefs->ClearPref(kDiscoverTabSuggestionChipTimesLeftToShow);
#endif

  // Added 02/2024.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  profile_prefs->ClearPref(kHatsSmartLockSurveyCycleEndTs);
  profile_prefs->ClearPref(kHatsSmartLockDeviceIsSelected);
  profile_prefs->ClearPref(kHatsUnlockSurveyCycleEndTs);
  profile_prefs->ClearPref(kHatsUnlockDeviceIsSelected);
#endif

  // Added 02/2024
  profile_prefs->ClearPref(kResetCheckDefaultBrowser);

  // Added 02/2024
  profile_prefs->ClearPref(kOfferReaderMode);

  // Added 03/2024.
  profile_prefs->ClearPref(kPlusAddressLastFetchedTime);

  // Added 03/2024.
  profile_prefs->ClearPref(kPrivacySandboxApisEnabled);

  // Added 03/2024.
  profile_prefs->ClearPref(kDefaultSearchProviderChoicePendingDeprecated);

  // Added 02/2024, but DO NOT REMOVE after the usual year!
  // TODO(crbug.com/40282890): Remove ~one year after full launch.
  browser_sync::MaybeMigrateSyncingUserToSignedIn(profile_path, profile_prefs);

  // Please don't delete the following line. It is used by PRESUBMIT.py.
  // END_MIGRATE_OBSOLETE_PROFILE_PREFS

  // IMPORTANT NOTE: This code is *not* run on iOS Chrome. If a pref is migrated
  // or cleared here, and that pref is also used in iOS Chrome, it may also need
  // to be migrated or cleared specifically for iOS as well. This could be by
  // doing the migration in feature code that's called by all platforms instead
  // of here, or by calling migration code in the appropriate place for iOS
  // specifically, e.g. ios/chrome/browser/shared/model/prefs/browser_prefs.mm.
}
