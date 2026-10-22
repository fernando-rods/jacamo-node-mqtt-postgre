-- =====================================================
-- Banco de dados WoT / JaCaMo
-- Esquema inicial de persistência
-- =====================================================

CREATE TABLE IF NOT EXISTS observations (
    id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,

    thing_id TEXT NOT NULL,
    property_name TEXT NOT NULL,

    numeric_value DOUBLE PRECISION,
    text_value TEXT,

    payload JSONB NOT NULL,

    topic TEXT NOT NULL,

    observed_at TIMESTAMPTZ NOT NULL
        DEFAULT CURRENT_TIMESTAMP,

    CONSTRAINT observations_value_check
        CHECK (
            numeric_value IS NOT NULL
            OR text_value IS NOT NULL
            OR payload IS NOT NULL
        )
);


CREATE INDEX IF NOT EXISTS idx_observations_thing_property_time
    ON observations (
        thing_id,
        property_name,
        observed_at DESC
    );


CREATE INDEX IF NOT EXISTS idx_observations_time
    ON observations (observed_at DESC);


-- =====================================================
-- Eventos recebidos dos dispositivos
-- =====================================================

CREATE TABLE IF NOT EXISTS device_events (
    id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,

    thing_id TEXT NOT NULL,
    event_name TEXT NOT NULL,

    payload JSONB NOT NULL,

    topic TEXT NOT NULL,

    received_at TIMESTAMPTZ NOT NULL
        DEFAULT CURRENT_TIMESTAMP
);


CREATE INDEX IF NOT EXISTS idx_device_events_thing_time
    ON device_events (
        thing_id,
        received_at DESC
    );


-- =====================================================
-- Ações solicitadas pelos agentes
-- =====================================================

CREATE TABLE IF NOT EXISTS agent_actions (
    id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,

    agent_id TEXT NOT NULL,
    thing_id TEXT NOT NULL,
    action_name TEXT NOT NULL,

    input JSONB,

    status TEXT NOT NULL DEFAULT 'requested',

    requested_at TIMESTAMPTZ NOT NULL
        DEFAULT CURRENT_TIMESTAMP,

    completed_at TIMESTAMPTZ,

    result JSONB,

    CONSTRAINT agent_actions_status_check
        CHECK (
            status IN (
                'requested',
                'confirmed',
                'failed',
                'unknown'
            )
        )
);


CREATE INDEX IF NOT EXISTS idx_agent_actions_thing_time
    ON agent_actions (
        thing_id,
        requested_at DESC
    );


CREATE INDEX IF NOT EXISTS idx_agent_actions_status
    ON agent_actions (status);
